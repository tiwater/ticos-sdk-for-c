//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/config.h"
#include "ticos/core/math.h"
#include "ticos/panics/platform/coredump.h"

#include "sdk_defs.h"

#ifndef TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY
#define TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY 1
#endif

#if !TICOS_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM
const sTcsCoredumpRegion *ticos_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  static sTcsCoredumpRegion s_coredump_regions[1];

  int region_idx = 0;

  // first, capture the stack that was active at the time of crash
  const size_t active_stack_size_to_collect = TICOS_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT;
  s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
    crash_info->stack_address,
    ticos_platform_sanitize_address_range(crash_info->stack_address,
                                             active_stack_size_to_collect));

  *num_regions = region_idx;
  return &s_coredump_regions[0];
}
#endif

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  const uint32_t ram_start = MEMORY_SYSRAM_BASE;
  const uint32_t ram_end = MEMORY_SYSRAM_END;

  if ((uint32_t)start_addr >= ram_start && (uint32_t)start_addr < ram_end) {
    return TICOS_MIN(desired_size, ram_end - (uint32_t)start_addr);
  }
  return 0;
}
