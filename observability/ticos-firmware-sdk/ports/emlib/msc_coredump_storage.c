//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions to use sectors of internal flash
//! on the EFM/EFR Memory System Controller.
//!
//! To use, update your linker script (.ld file) to expose information about the location to use.
//! For example, using the last 64K of the EFM32PG12BxxxF1024 (1MB flash) would look
//! something like this:
//!
//! MEMORY
//! {
//!    /* ... other regions ... */
//!    COREDUMP_STORAGE_FLASH (rx) : ORIGIN = 0xF0000, LENGTH = 64K
//! }
//! __TicosCoreStorageStart = ORIGIN(COREDUMP_STORAGE_FLASH);
//! __TicosCoreStorageEnd = ORIGIN(COREDUMP_STORAGE_FLASH) + LENGTH(COREDUMP_STORAGE_FLASH);

#include "ticos/config.h"

#if TICOS_PLATFORM_COREDUMP_STORAGE_USE_FLASH

#include "ticos/panics/coredump.h"
#include "ticos/ports/buffered_coredump_storage.h"

#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/panics/platform/coredump.h"

#include "em_device.h"
#include "em_msc.h"

#ifndef TICOS_COREDUMP_STORAGE_START_ADDR
extern uint32_t __TicosCoreStorageStart[];
#define TICOS_COREDUMP_STORAGE_START_ADDR ((uint32_t)__TicosCoreStorageStart)
#endif

#ifndef TICOS_COREDUMP_STORAGE_END_ADDR
extern uint32_t __TicosCoreStorageEnd[];
#define TICOS_COREDUMP_STORAGE_END_ADDR ((uint32_t)__TicosCoreStorageEnd)
#endif

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

void ticos_platform_coredump_storage_clear(void) {
  uint32_t *addr = (void *)TICOS_COREDUMP_STORAGE_START_ADDR;
  uint32_t zeros = 0;

  const MSC_Status_TypeDef rv = MSC_WriteWord(addr, &zeros, sizeof(zeros));
  if ((rv != mscReturnOk) || ((*addr) != 0)) {
    TICOS_LOG_ERROR("Failed to clear coredump storage, rv=%d", (int)rv);
  }
}

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  const size_t size =
      TICOS_COREDUMP_STORAGE_END_ADDR - TICOS_COREDUMP_STORAGE_START_ADDR;

  *info  = (sTcsCoredumpStorageInfo) {
    .size = size,
    .sector_size = FLASH_PAGE_SIZE,
  };
}

bool ticos_platform_coredump_storage_buffered_write(sCoredumpWorkingBuffer *blk) {
  const uint32_t addr = TICOS_COREDUMP_STORAGE_START_ADDR + blk->write_offset;

  const MSC_Status_TypeDef rv = MSC_WriteWord((void *)addr, blk->data, sizeof(blk->data));
  if (rv != mscReturnOk) {
    prv_coredump_writer_assert_and_reboot(rv);
  }

  return true;
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

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  if (((offset % FLASH_PAGE_SIZE) != 0) || (erase_size % FLASH_PAGE_SIZE) != 0) {
    // configuration error
    prv_coredump_writer_assert_and_reboot(offset);
  }

  const uint32_t start_addr = TICOS_COREDUMP_STORAGE_START_ADDR + offset;

  for (size_t sector_offset = 0; sector_offset < erase_size; sector_offset += FLASH_PAGE_SIZE) {
    const MSC_Status_TypeDef rv = MSC_ErasePage((void *)(start_addr + sector_offset));
    if (rv != mscReturnOk) {
      prv_coredump_writer_assert_and_reboot(rv);
    }
  }

  return true;
}

#endif /* TICOS_PLATFORM_COREDUMP_STORAGE_USE_FLASH */
