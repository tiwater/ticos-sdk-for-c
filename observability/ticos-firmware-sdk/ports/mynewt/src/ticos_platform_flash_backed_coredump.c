//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Implements platform dependencies for storing Ticos coredumps in mynewt coredump storage.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flash_map/flash_map.h"
#include "hal/hal_bsp.h"
#include "img_mgmt/img_mgmt.h"
#include "syscfg/syscfg.h"
#include "sysflash/sysflash.h"

#include "ticos/components.h"

#if MYNEWT_VAL(TICOS_ENABLE)

const sTcsCoredumpRegion *
ticos_platform_coredump_get_regions(const sCoredumpCrashInfo *crash_info,
                                       size_t *num_regions) {
  static sTcsCoredumpRegion s_coredump_regions[MYNEWT_VAL(TICOS_COREDUMP_MAX_AREA_COUNT)];

#if MYNEWT_VAL(TICOS_COREDUMP_CAPTURE_MINIMAL)
  const size_t active_stack_size_to_collect = TICOS_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT;
  s_coredump_regions[0] = TICOS_COREDUMP_MEMORY_REGION_INIT(
    crash_info->stack_address,
    ticos_platform_sanitize_address_range(crash_info->stack_address,
                                             active_stack_size_to_collect));

  *num_regions = 1;
#else
  int area_cnt = 0;
  const struct hal_bsp_mem_dump *mem = hal_bsp_core_dump(&area_cnt);

  const size_t areas_to_collect = TICOS_MIN(TICOS_ARRAY_SIZE(s_coredump_regions),
                                            area_cnt);
  for (size_t i = 0; i < areas_to_collect; i++) {
    s_coredump_regions[i] =
        TICOS_COREDUMP_MEMORY_REGION_INIT(mem[i].hbmd_start, mem[i].hbmd_size);
  }

  *num_regions = areas_to_collect;
#endif

  return s_coredump_regions;
}

static int prv_flash_open(const struct flash_area **fa) {
  if (flash_area_open(MYNEWT_VAL(COREDUMP_FLASH_AREA), fa)) {
    return -OS_ERROR;
  }

  // Don't overwrite an image that has any flags set (pending, active, or confirmed).
  const int slot = flash_area_id_to_image_slot(MYNEWT_VAL(COREDUMP_FLASH_AREA));
  if (slot != -1 && img_mgmt_slot_in_use(slot)) {
    return -OS_ERROR;
  }

  return OS_OK;
}

void ticos_platform_coredump_storage_get_info(
    sTcsCoredumpStorageInfo *info) {
  const struct flash_area *fa;

  if (prv_flash_open(&fa)) {
    assert(0);
  }

  *info = (sTcsCoredumpStorageInfo){
    .size = fa->fa_size,
    .sector_size = 0, // Not needed for port
  };
}

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  const struct flash_area *fa;

  if (prv_flash_open(&fa)) {
    return false;
  }

  if ((offset + read_len) > fa->fa_size) {
    return false;
  }

  if (flash_area_read(fa, offset, data, read_len)) {
    return false;
  }

  return true;
}

bool ticos_platform_coredump_storage_erase(uint32_t offset,
                                              size_t erase_size) {
  const struct flash_area *fa;

  if (prv_flash_open(&fa)) {
    return false;
  }

  if ((offset + erase_size) > fa->fa_size) {
    return false;
  }

  if (flash_area_erase(fa, offset, erase_size)) {
    return false;
  }

  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  const struct flash_area *fa;

  if (prv_flash_open(&fa)) {
    return false;
  }

  if ((offset + data_len) > fa->fa_size) {
    return false;
  }

  if (flash_area_write(fa, offset, data, data_len)) {
    return false;
  }

  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  const struct flash_area *fa;

  if (prv_flash_open(&fa)) {
    assert(0);
  }

  // Erasing whole area takes too much time and causes ble connection to time out. Instead only
  // clear the first byte to mark the segment as invalid.
  const uint8_t clear_byte = 0x0;
  ticos_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}

#endif /* MYNEWT_VAL(TICOS_ENABLE) */
