#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Background: Many storage implementations have alignment requirements for write / program
//! operations. Reasons vary but include:
//!  - A Flash storage is covered by ECC bits meaning a block can only get written once since
//!    the ECC bits are in flash too.
//!  - Memory region can only program in units of words, double-words, etc.
//!
//! This is a single-file header style utility that can be included in a flash coredump storage
//! implementation to buffer write operations so they are guaranteed be aligned along and sized in
//! TICOS_COREDUMP_STORAGE_WRITE_SIZE units.
//!
//! The implementation takes advantage of the property that the Ticos SDK will always use
//! sequential writes when flushing to coredump storage with the exception of the header which is
//! written at offset 0 of storage as the last step.
//!
//! To use:
//!
//! 1. Include this file in your storage port
//! 2. Implement buffered writer:
//!   bool ticos_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *block) {
//!      const addr = your_storage_base_addr + block->offset;
//!      your_storage_write(addr, block->data, TICOS_COREDUMP_STORAGE_WRITE_SIZE);
//!   }
//!
//! Usage Notes:
//!  - The size of coredump storage itself must be >= TICOS_COREDUMP_STORAGE_WRITE_SIZE
//!  - If a coredump is not exactly TICOS_COREDUMP_STORAGE_WRITE_SIZE units in size, a
//!    TICOS_COREDUMP_STORAGE_WRITE_SIZE size buffer will still be written but unused bytes
//!    will be set to zero.

#include <stdint.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/math.h"
#include "ticos/panics/platform/coredump.h"

#ifndef TICOS_COREDUMP_STORAGE_WRITE_SIZE
# define TICOS_COREDUMP_STORAGE_WRITE_SIZE 32
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CoredumpWorkingBuffer sCoredumpWorkingBuffer;

//! Callback invoked when a 'TICOS_COREDUMP_STORAGE_WRITE_SIZE' is ready to be written
//!
//! @param blk The block of data to write. The size to write is always
//!  TICOS_COREDUMP_STORAGE_WRITE_SIZE.
//!
//! @return true if write was successful, false otherwise
bool ticos_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk);

//
// Buffered Storage Implementation, single-file header style so it can easily be picked up
// by a coredump storage port by simply including this file.
//

struct CoredumpWorkingBuffer {
  // data to write
  uint8_t data[TICOS_COREDUMP_STORAGE_WRITE_SIZE];
  // offset within storage to be written to
  uint32_t write_offset;

  // Internal tracking of how many bytes were written
  uint32_t bytes_written;
};

TICOS_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer_header;
TICOS_ALIGNED(8) static sCoredumpWorkingBuffer s_working_buffer;

static sCoredumpWorkingBuffer *prv_get_working_buf(uint32_t offset) {
  return (offset == 0) ? &s_working_buffer_header : &s_working_buffer;
}

static bool prv_write_blk(sCoredumpWorkingBuffer *block) {
  if (!ticos_platform_coredump_storage_buffered_write(block)) {
    return false;
  }
  *block = (sCoredumpWorkingBuffer){ 0 };
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

static bool ticos_coredump_storage_buffered_write(uint32_t offset, const void *data,
                                                     size_t data_len) {
  sTcsCoredumpStorageInfo info = { 0 };
  ticos_platform_coredump_storage_get_info(&info);
  if (((offset + data_len) > info.size) ||
      ((info.size % TICOS_COREDUMP_STORAGE_WRITE_SIZE) != 0) ||
      (info.size < TICOS_COREDUMP_STORAGE_WRITE_SIZE)) {
    return false; // out of bounds write
  }

  const uint8_t *datap = (const uint8_t *)data;
  uint32_t start_addr = offset;
  uint32_t page_aligned_start_address =
      (start_addr / TICOS_COREDUMP_STORAGE_WRITE_SIZE) * TICOS_COREDUMP_STORAGE_WRITE_SIZE;

  // we have to copy data into a temporary buffer because we can only issue aligned writes
  if (page_aligned_start_address != start_addr) {
    sCoredumpWorkingBuffer *working_buffer =
        prv_get_working_buf(page_aligned_start_address);
    uint32_t bytes_to_write = TICOS_MIN(
        (page_aligned_start_address + TICOS_COREDUMP_STORAGE_WRITE_SIZE) - start_addr,
        data_len);
    uint32_t write_offset = start_addr - page_aligned_start_address;
    memmove(&working_buffer->data[write_offset], datap, bytes_to_write);
    working_buffer->bytes_written += bytes_to_write;
    working_buffer->write_offset = page_aligned_start_address;
    if (!prv_try_flush()) {
      return false;
    }

    start_addr += bytes_to_write;
    data_len -= bytes_to_write;
    datap += bytes_to_write;
  }

  // now that we have copied data into a temporary buffer and are aligned by the expected
  // write size, let's flush complete blocks to flash
  for (uint32_t i = 0; i < data_len; i += TICOS_COREDUMP_STORAGE_WRITE_SIZE) {
    const uint32_t size =
        TICOS_MIN(TICOS_COREDUMP_STORAGE_WRITE_SIZE, data_len - i);
    sCoredumpWorkingBuffer *working_buffer =
        prv_get_working_buf(start_addr + i);
    memmove(&working_buffer->data, &datap[i], size);
    working_buffer->bytes_written += size;
    working_buffer->write_offset = start_addr + i;
    if (!prv_try_flush()) {
      return false;
    }
  }

  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data, size_t data_len) {
  return ticos_coredump_storage_buffered_write(offset, data, data_len);
}

#ifdef __cplusplus
}
#endif
