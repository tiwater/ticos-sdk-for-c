//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

#include "ticos/panics/platform/coredump.h"
#include "ticos/panics/arch/arm/cortex_m.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/math.h"

TICOS_PUT_IN_SECTION(".noinit.tcs_coredump") TICOS_ALIGNED(8)
static uint8_t s_ram_backed_coredump_region[CONFIG_TICOS_RAM_BACKED_COREDUMP_SIZE];

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  *info = (sTcsCoredumpStorageInfo) {
    .size = sizeof(s_ram_backed_coredump_region),
    .sector_size = sizeof(s_ram_backed_coredump_region),
  };
}

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((offset + read_len) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  const uint8_t *read_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(data, read_ptr, read_len);
  return true;
}


bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  uint8_t *erase_ptr = &s_ram_backed_coredump_region[offset];
  memset(erase_ptr, 0x0, erase_size);
  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > sizeof(s_ram_backed_coredump_region)) {
    return false;
  }

  uint8_t *write_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  ticos_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}
