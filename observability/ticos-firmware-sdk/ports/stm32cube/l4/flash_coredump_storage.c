//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions to use a sectors of internal flash
//! on the STM32L4 family for coredump capture.
//!
//! To use, update your linker script (.ld file) to expose information about the location to use.
//!
//! For example, using the last 64kB (32 sectors) of the STM32L475VGT (1MB dual banked flash) would
//! look something like this:
//!
//! MEMORY
//! {
//!    /* ... other regions ... */
//!    COREDUMP_STORAGE_FLASH (rx) : ORIGIN = 0x80f0000, LENGTH = 64K
//! }
//! __TicosCoreStorageStart = ORIGIN(COREDUMP_STORAGE_FLASH);
//! __TicosCoreStorageEnd = ORIGIN(COREDUMP_STORAGE_FLASH) + LENGTH(COREDUMP_STORAGE_FLASH);
//!
//! Notes:
//!  - STM32L4 internal flash is contiguous and every sector has the same sector size
//!  - __TicosCoreStorageStart & __TicosCoreStorageEnd must be aligned on sector
//!    boundaries

#include "ticos/panics/coredump.h"
#include "ticos/ports/buffered_coredump_storage.h"

#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/panics/platform/coredump.h"
#include "ticos/ports/stm32cube/l4/flash.h"

#include "stm32l4xx_hal.h"

#ifndef TICOS_COREDUMP_STORAGE_START_ADDR
extern uint32_t __TicosCoreStorageStart[];
#define TICOS_COREDUMP_STORAGE_START_ADDR ((uint32_t)__TicosCoreStorageStart)
#endif

#ifndef TICOS_COREDUMP_STORAGE_END_ADDR
extern uint32_t __TicosCoreStorageEnd[];
#define TICOS_COREDUMP_STORAGE_END_ADDR ((uint32_t)__TicosCoreStorageEnd)
#endif

bool ticos_stm32cubel4_flash_clear_ecc_error(
    uint32_t start_addr, uint32_t end_addr, uint32_t *corrupted_address) {
  const bool eccd_error = __HAL_FLASH_GET_FLAG(FLASH_FLAG_ECCD);
  if (!eccd_error) {
    if (corrupted_address != NULL) {
      *corrupted_address = 0; // no error found
    }
    return 0;
  }

  const uint32_t eccr = FLASH->ECCR;
  uint32_t corrupted_flash_address =
      FLASH_BASE + ((eccr & FLASH_ECCR_ADDR_ECC) >> FLASH_ECCR_ADDR_ECC_Pos);

  // if the STM32L4 is dual-banked check to see what bank has a bit corrupted
#if defined(FLASH_OPTR_BFB2)
  if ((eccr & FLASH_ECCR_BK_ECC) != 0) {
    corrupted_flash_address += FLASH_BANK_SIZE;
  }
#endif

  if (corrupted_address != NULL) {
      *corrupted_address = corrupted_flash_address;
  }

  if (corrupted_flash_address < start_addr ||
      corrupted_flash_address > end_addr) {
    // There is a ECC error but it is in a range we do not want to zero out
    return -1;
  }

  // NB: The STM32L4 allows for a double word to reprogrammed to 0x0. If the word
  // had an ECC error for some reason, this will also clear the ECC bits and the error
  // on the word

  uint32_t res;
  HAL_FLASH_Unlock();
  {
      uint64_t clear_error = 0;
      res = HAL_FLASH_Program(
          FLASH_TYPEPROGRAM_DOUBLEWORD, corrupted_flash_address, clear_error);
  }
  HAL_FLASH_Lock();
  return res == HAL_OK ? 0 : res;
}

void ticos_platform_fault_handler(const sTcsRegState *regs, eTicosRebootReason reason) {
  ticos_stm32cubel4_flash_clear_ecc_error(
      TICOS_COREDUMP_STORAGE_START_ADDR, TICOS_COREDUMP_STORAGE_END_ADDR, NULL);

}

// Error writing to flash - should never happen & likely detects a configuration error
// Call the reboot handler which will halt the device if a debugger is attached and then reboot
TICOS_NO_OPT
static void prv_coredump_writer_assert_and_reboot(int error_code) {
  ticos_platform_halt_if_debugging();
  ticos_platform_reboot();
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sTcsCoredumpStorageInfo info = { 0 };
  ticos_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  const size_t size =
      TICOS_COREDUMP_STORAGE_END_ADDR - TICOS_COREDUMP_STORAGE_START_ADDR;

  *info  = (sTcsCoredumpStorageInfo) {
    .size = size,
    // The STM32L4 series has a fixed page size and a contiguous address layout
    .sector_size = FLASH_PAGE_SIZE,
  };
}

// NOTE: The internal STM32L4 flash uses 8 ECC bits over 8 byte "memory words". Since the ECC
// bits are also in NOR flash, this means 8 byte hunks can only be written once since changing
// a value in the double word after the fact would cause the ECC to fail.
//
// In practice this means, writes must be issued 8 bytes at a time. The code below accomplishes
// this by buffering writes and then flushing in 8 byte hunks. The Ticos coredump writer is
// guaranteed to issue writes sequentially with the exception of the header which is at the
// beginning of the coredump region and written last.

bool ticos_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  const uint32_t start_addr = TICOS_COREDUMP_STORAGE_START_ADDR;
  const uint32_t addr = start_addr + blk->write_offset;

  HAL_FLASH_Unlock();
  {
    uint64_t data;
    for (size_t i = 0; i < TICOS_COREDUMP_STORAGE_WRITE_SIZE; i += sizeof(data)) {
      memcpy(&data, &blk->data[i], sizeof(data));

      const uint32_t res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + i, data);

      if (res != HAL_OK) {
        prv_coredump_writer_assert_and_reboot(res);
      }
    }
  }
  HAL_FLASH_Lock();

  *blk = (sCoredumpWorkingBuffer){ 0 };
  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  HAL_FLASH_Unlock();
  {
    const uint64_t clear_val = 0x0;
    const uint32_t res = HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_DOUBLEWORD, TICOS_COREDUMP_STORAGE_START_ADDR, clear_val);

    if (res != HAL_OK) {
      TICOS_LOG_ERROR("Could not clear coredump storage, 0x%" PRIx32, res);
    }
  }
  HAL_FLASH_Lock();
}

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  // The internal flash is memory mapped so we can just use memcpy!
  const uint32_t start_addr = TICOS_COREDUMP_STORAGE_START_ADDR;
  memcpy(data, (void *)(start_addr + offset), read_len);
  return true;
}

static bool prv_erase_from_bank(uint32_t Banks, uint32_t Page, uint32_t NbPages) {
  FLASH_EraseInitTypeDef s_erase_cfg = {
    .TypeErase = FLASH_TYPEERASE_PAGES,
    .Banks = Banks,
    .Page = Page,
    .NbPages = NbPages,
  };
  uint32_t SectorError = 0;
  HAL_FLASH_Unlock();
  {
    int res = HAL_FLASHEx_Erase(&s_erase_cfg, &SectorError);
    if (res != HAL_OK) {
      prv_coredump_writer_assert_and_reboot(res);
    }
  }
  HAL_FLASH_Lock();
  return true;
}

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  const size_t stm32_l4_flash_end_addr = (FLASH_END + 1);
  const size_t stm32_l4_flash_bank1_end_addr = (FLASH_BANK1_END + 1);

  // check that address is in the range of flash
  uint32_t erase_begin_addr = TICOS_COREDUMP_STORAGE_START_ADDR + offset;
  const uint32_t end_addr = erase_begin_addr + erase_size;
  if (erase_begin_addr < FLASH_BASE || erase_begin_addr > stm32_l4_flash_end_addr) {
    return false;
  }

  // make sure region is aligned along page boundaries and
  // is whole page units in size
  if (((erase_begin_addr + offset) % FLASH_PAGE_SIZE) != 0 ||
      (erase_size % FLASH_PAGE_SIZE) != 0) {
    return false;
  }

  if (erase_begin_addr < stm32_l4_flash_bank1_end_addr) {
    const uint32_t bank1_erase_end_addr = TICOS_MIN(stm32_l4_flash_bank1_end_addr, end_addr);

    const size_t bank1_page_start = (erase_begin_addr - FLASH_BASE) / FLASH_PAGE_SIZE;
    const size_t bank1_num_pages = (bank1_erase_end_addr - erase_begin_addr) / FLASH_PAGE_SIZE;

    if (!prv_erase_from_bank(FLASH_BANK_1, bank1_page_start,  bank1_num_pages)) {
      return false;
    }

    if (bank1_erase_end_addr == end_addr) {
      return true;
    }

    // there's erasing to do in bank 2
    erase_begin_addr = bank1_erase_end_addr;
  }

  const size_t bank2_page_start =
      (erase_begin_addr - stm32_l4_flash_bank1_end_addr) / FLASH_PAGE_SIZE;
  const size_t bank2_num_pages =
      (end_addr - erase_begin_addr) / FLASH_PAGE_SIZE;

  return prv_erase_from_bank(FLASH_BANK_2, bank2_page_start,  bank2_num_pages);
}
