//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A port for the platform dependencies needed to use the coredump feature from the "panics"
//! component by saving the Ticos coredump data in a "noinit" region of RAM.
//!
//! This can be linked in directly by adding the .c file to the build system or can be
//! copied into your repo and modified to collect different RAM regions.
//!
//! By default, it will collect the top of the stack which was running at the time of the
//! crash. This allows for a reasonable backtrace to be collected while using very little RAM.
//!
//! Place the "noinit" region in an area of RAM that will persist across bootup.
//!    The region must:
//!    - not be placed in .bss
//!    - not be an area of RAM used by any of your bootloaders
//!    For example, with GNU GCC, this can be achieved by adding something like the following to
//!    your linker script:
//!    MEMORY
//!    {
//!      [...]
//!      COREDUMP_NOINIT (rw) :  ORIGIN = <RAM_REGION_START>, LENGTH = 1024
//!    }
//!    SECTIONS
//!    {
//!      [...]
//!      .coredump_noinit (NOLOAD): { KEEP(*(*.noinit.tcs_coredump)) } > COREDUMP_NOINIT
//!    }

#include "ticos/config.h"

#if TICOS_PLATFORM_COREDUMP_STORAGE_USE_RAM

#include "ticos/panics/platform/coredump.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/math.h"

#if !TICOS_PLATFORM_COREDUMP_STORAGE_RAM_CUSTOM

#if ((TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE % 4) != 0)
#error "TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE must be a multiple of 4"
#endif

TICOS_STATIC_ASSERT(sizeof(uint32_t) == 4, "port expects sizeof(uint32_t) == 4");

TICOS_PUT_IN_SECTION(TICOS_PLATFORM_COREDUMP_NOINIT_SECTION_NAME)
static uint32_t s_ram_backed_coredump_region[TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE / 4];

#define TICOS_PLATFORM_COREDUMP_RAM_START_ADDR ((uint8_t *)&s_ram_backed_coredump_region[0])

#endif /* TICOS_PLATFORM_COREDUMP_STORAGE_RAM_CUSTOM */

#if !TICOS_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM
//! Collect the active stack as part of the coredump capture.
//! User can implement their own version to override the implementation
TICOS_WEAK
const sTcsCoredumpRegion *ticos_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
   static sTcsCoredumpRegion s_coredump_regions[1];

   const size_t stack_size = ticos_platform_sanitize_address_range(
       crash_info->stack_address, TICOS_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT);

   s_coredump_regions[0] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       crash_info->stack_address, stack_size);
   *num_regions = TICOS_ARRAY_SIZE(s_coredump_regions);
   return &s_coredump_regions[0];
}
#endif

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  *info = (sTcsCoredumpStorageInfo) {
    .size = TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE,
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

  const uint8_t *storage_ptr = TICOS_PLATFORM_COREDUMP_RAM_START_ADDR;
  const uint8_t *read_ptr = &storage_ptr[offset];
  memcpy(data, read_ptr, read_len);
  return true;
}

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if (!prv_op_within_flash_bounds(offset, erase_size)) {
    return false;
  }

  uint8_t *storage_ptr = TICOS_PLATFORM_COREDUMP_RAM_START_ADDR;
  void *erase_ptr = &storage_ptr[offset];
  memset(erase_ptr, 0x0, erase_size);
  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if (!prv_op_within_flash_bounds(offset, data_len)) {
    return false;
  }

  uint8_t *storage_ptr = TICOS_PLATFORM_COREDUMP_RAM_START_ADDR;
  uint8_t *write_ptr = (uint8_t *)&storage_ptr[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  ticos_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}

#endif /* TICOS_PLATFORM_COREDUMP_STORAGE_USE_RAM */
