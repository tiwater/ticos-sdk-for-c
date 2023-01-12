//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Reference implementation of platform dependency functions to use space on the
//! external SPI flash connected to the DA145xx for coredump capture.
//!
//! To use, update "ticos_platform_config.h" with a _free_ address range on the NOR flash to
//! capture the coredump. The size provisioned should be >= the size of RAM and be aligned on
//! sector boundaries. For example:
//!
//! #define TICOS_COREDUMP_STORAGE_START_ADDR 0x20000
//! #define TICOS_COREDUMP_STORAGE_END_ADDR 0x30000

#include "ticos/config.h"

#if TICOS_PLATFORM_COREDUMP_STORAGE_USE_FLASH

#include "ticos/panics/platform/coredump.h"

#include <stdbool.h>
#include <stdint.h>

#include "arch_wdg.h"
#include "spi_flash.h"

#if !defined(TICOS_COREDUMP_STORAGE_START_ADDR) || !defined(TICOS_COREDUMP_STORAGE_END_ADDR)
#error "TICOS_COREDUMP_STORAGE_START_ADDR & TICOS_COREDUMP_STORAGE_END_ADDR must be specified in ticos_platform_config.h"
#endif

#if ((TICOS_COREDUMP_STORAGE_START_ADDR % SPI_FLASH_SECTOR_SIZE) != 0)
#error "TICOS_COREDUMP_STORAGE_START_ADDR should be aligned by the sector size"
#endif

#if ((TICOS_COREDUMP_STORAGE_END_ADDR % SPI_FLASH_SECTOR_SIZE) != 0)
#error "TICOS_COREDUMP_STORAGE_END_ADDR should be aligned by the sector size"
#endif

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  *info  = (sTcsCoredumpStorageInfo) {
    .size = TICOS_COREDUMP_STORAGE_END_ADDR - TICOS_COREDUMP_STORAGE_START_ADDR,
    .sector_size = SPI_FLASH_SECTOR_SIZE,
  };
}

static bool prv_op_within_flash_bounds(uint32_t offset, size_t data_len) {
  sTcsCoredumpStorageInfo info = { 0 };
  ticos_platform_coredump_storage_get_info(&info);
  return (offset + data_len) <= info.size;
}

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if (!prv_op_within_flash_bounds(offset, read_len)) {
    return false;
  }

  const uint32_t address = TICOS_COREDUMP_STORAGE_START_ADDR + offset;

  // Flash operations are blocking and can take some time...
  wdg_reload(WATCHDOG_DEFAULT_PERIOD);

  uint32_t actual_size = 0;
  if (spi_flash_read_data((uint8_t *)data, address, (uint32_t)read_len,
                          &actual_size) != SPI_FLASH_ERR_OK) {
    return false;
  }

  return (actual_size == read_len);
}

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  const size_t sector_size = SPI_FLASH_SECTOR_SIZE;

  if ((offset % sector_size) != 0) {
    return false;
  }

  // Flash operations are blocking and can take some time...
  wdg_reload(WATCHDOG_DEFAULT_PERIOD);

  for (size_t sector = offset; sector < erase_size; sector += sector_size) {
    const uint32_t address = TICOS_COREDUMP_STORAGE_START_ADDR + sector;
    if (spi_flash_block_erase(address, SPI_FLASH_OP_SE) != SPI_FLASH_ERR_OK) {
      return false;
    }
  }

  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if (!prv_op_within_flash_bounds(offset, data_len)) {
    return false;
  }

  const uint32_t address = TICOS_COREDUMP_STORAGE_START_ADDR + offset;

  // Flash operations are blocking and can take some time...
  wdg_reload(WATCHDOG_DEFAULT_PERIOD);

  uint32_t actual_size = 0;
  if (spi_flash_write_data((uint8_t *)data, address, (uint32_t)data_len,
                           &actual_size) != SPI_FLASH_ERR_OK) {
    return false;
  }

  return (actual_size == data_len);
}

// Note: This function is called while the system is running once the coredump has been read. We
// clear the first word in this scenario to avoid blocking the system for a long time on an erase.
void ticos_platform_coredump_storage_clear(void) {
  uint32_t clear_word = 0x0;
  ticos_platform_coredump_storage_write(0, &clear_word, sizeof(clear_word));
}

#endif /* TICOS_PLATFORM_COREDUMP_STORAGE_USE_FLASH */
