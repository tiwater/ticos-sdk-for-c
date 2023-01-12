//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Persistent storage using chip internal flash.
//!

#include "ticos_platform_storage.h"

#include <stdio.h>

#include "ticos/components.h"

/* Flash page size */
#define TICOS_COREDUMP_STORAGE_WRITE_SIZE 512

/* Coredump working buffer */
typedef struct {
  uint8_t data[TICOS_COREDUMP_STORAGE_WRITE_SIZE];
  uint32_t address;
  uint32_t bytes_written;
} sCoredumpWorkingBuffer;

/* Flash block object, initialized in main.c. */
cyhal_flash_t flash_obj;
cyhal_flash_info_t flash_obj_info;

/* Section addresses from the linker script. */
extern uint8_t __bss_start__;
extern uint8_t __bss_end__;

extern uint8_t __data_start__;
extern uint8_t __data_end__;

extern uint8_t __StackLimit;
extern uint8_t __StackTop;

TICOS_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer_header;
TICOS_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer;

static sCoredumpWorkingBuffer *prv_get_working_buf(uint32_t offset) {
  return (offset == 0) ? &s_working_buffer_header : &s_working_buffer;
}

static bool prv_write_blk(sCoredumpWorkingBuffer *blk) {
  const uint32_t start_addr = (uint32_t)&__TcsCoredumpsStart;
  const uint32_t addr = start_addr + blk->address;

  cy_rslt_t result = cyhal_flash_write(&flash_obj, addr, (uint32_t *)blk->data);

  if (result != CY_RSLT_SUCCESS) {
    return false;
  }

  *blk = (sCoredumpWorkingBuffer){0};

  return true;
}

static bool prv_try_flush(void) {
  sCoredumpWorkingBuffer *hdr_block = &s_working_buffer_header;
  sCoredumpWorkingBuffer *data_block = &s_working_buffer;

  if (hdr_block->bytes_written == TICOS_COREDUMP_STORAGE_WRITE_SIZE) {
    // this is the final write for the coredump (header)

    // it's possible the last data blob in the coredump isn't a multiple
    // of our write size so let's flush whatever is queued up. (Unused bytes
    // are just zero'd since the working buffer is memset to 0 before each use)
    if (data_block->bytes_written != 0) {
      if (!prv_write_blk(data_block)) {
        return false;
      }
    }

    // write the header
    if (!prv_write_blk(hdr_block)) {
      return false;
    }
  }

  if (data_block->bytes_written == TICOS_COREDUMP_STORAGE_WRITE_SIZE) {
    if (!prv_write_blk(data_block)) {
      return false;
    }
  }

  return true;
}

const sTcsCoredumpRegion *ticos_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  static sTcsCoredumpRegion s_coredump_regions[3];

  uint32_t bss_size = &__bss_end__ - &__bss_start__;
  uint32_t data_size = &__data_end__ - &__data_start__;
  uint32_t stack_size = &__StackTop - &__StackLimit;

  s_coredump_regions[0] = TICOS_COREDUMP_MEMORY_REGION_INIT(&__data_start__, data_size);
  s_coredump_regions[1] = TICOS_COREDUMP_MEMORY_REGION_INIT(&__bss_start__, bss_size);
  s_coredump_regions[2] = TICOS_COREDUMP_MEMORY_REGION_INIT(&__StackLimit, stack_size);

  *num_regions = TICOS_ARRAY_SIZE(s_coredump_regions);

  return s_coredump_regions;
}

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  /* Size of the coredumps region based on addresses from the linker script. */
  uint32_t coredumps_sz = (&__TcsCoredumpsEnd - &__TcsCoredumpsStart);

  /* We're in the first block only, so get the page size from the first block. */
  *info = (sTcsCoredumpStorageInfo){
    .size = coredumps_sz,
    .sector_size = flash_obj_info.blocks[0].page_size,
  };
}

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len) {
  cy_rslt_t result;

  if (&__TcsCoredumpsStart + offset + read_len > &__TcsCoredumpsEnd) {
    return false;
  }

  result = cyhal_flash_read(&flash_obj, (uint32_t)(&__TcsCoredumpsStart) + offset, (uint8_t *)data,
                            read_len);

  return result == CY_RSLT_SUCCESS;
}

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (&__TcsCoredumpsStart + offset + erase_size > &__TcsCoredumpsEnd) {
    return false;
  }

  ticos_platform_coredump_storage_clear();

  return true;
}

/* Based on buffered_coredump_storage.h */
bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data, size_t data_len) {
  if (&__TcsCoredumpsStart + offset + data_len > &__TcsCoredumpsEnd) {
    return false;
  }

  const uint8_t *datap = data;
  uint32_t start_addr = offset;
  uint32_t page_aligned_start_address =
    (start_addr / TICOS_COREDUMP_STORAGE_WRITE_SIZE) * TICOS_COREDUMP_STORAGE_WRITE_SIZE;

  /* We have to copy data into a temporary buffer because we can only issue aligned writes. */
  if (page_aligned_start_address != start_addr) {
    sCoredumpWorkingBuffer *working_buffer = prv_get_working_buf(page_aligned_start_address);
    uint32_t bytes_to_write = TICOS_MIN(
      (page_aligned_start_address + TICOS_COREDUMP_STORAGE_WRITE_SIZE) - start_addr, data_len);
    uint32_t write_offset = start_addr - page_aligned_start_address;
    memcpy(&working_buffer->data[write_offset], datap, bytes_to_write);
    working_buffer->bytes_written += bytes_to_write;
    working_buffer->address = page_aligned_start_address;
    if (!prv_try_flush()) {
      return false;
    }

    start_addr += bytes_to_write;
    data_len -= bytes_to_write;
    datap += bytes_to_write;
  }

  for (uint32_t i = 0; i < data_len; i += TICOS_COREDUMP_STORAGE_WRITE_SIZE) {
    const uint32_t size = TICOS_MIN(TICOS_COREDUMP_STORAGE_WRITE_SIZE, data_len - i);
    sCoredumpWorkingBuffer *working_buffer = prv_get_working_buf(start_addr + i);
    memcpy(&working_buffer->data, &datap[i], size);
    working_buffer->bytes_written += size;
    working_buffer->address = start_addr + i;
    if (!prv_try_flush()) {
      return false;
    }
  }

  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  uint32_t i;
  uint32_t page_addr;
  uint32_t coredumps_sz = (&__TcsCoredumpsEnd - &__TcsCoredumpsStart);
  uint32_t page_sz = TICOS_COREDUMP_STORAGE_WRITE_SIZE;
  uint32_t page_count = coredumps_sz / page_sz;

  /* Erase the whole coredumps region. */
  for (i = 0; i < page_count; i++) {
    page_addr = (uint32_t)(&__TcsCoredumpsStart) + (i * page_sz);
    cyhal_flash_erase(&flash_obj, page_addr);
  }
}
