//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Implements APIs for collecting RAM regions on nRF5 as part of a coredump
//!
//! Options (To override, update ticos_platform_config.h)
//! TICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS (default = 0)
//!  When set to 1, user must define ticos_platform_coredump_get_regions() and
//!  declare their own regions to collect.
//!
//!  TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY (default = 1)
//!   This mode will collect just the stack that was active at the time of
//!   crash. To capture more data, set this to 0 in ticos_platform_config.h.
//!   The regions to be captured are specified by adding the following to the
//!   project's .ld file:
//!
//!     __TicosCoredumpRamStart = ORIGIN(RAM);
//!     __TcsCoredumpRamEnd = ORIGIN(RAM) + LENGTH(RAM);
//!

#include "ticos/core/math.h"
#include "ticos/panics/platform/coredump.h"
#include "sdk_common.h"

#ifndef TICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS
  #define TICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS 0
#endif

//! Truncates the region if it's outside the bounds of RAM
size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  // All NRF MCUs RAM starts at this address. No define is exposed in the SDK for it however
  const uint32_t ram_start = 0x20000000;

#ifdef NRF51
  const uint32_t ram_size = (NRF_FICR->SIZERAMBLOCKS) * NRF_FICR->NUMRAMBLOCK;
#else
  const uint32_t ram_size = NRF_FICR->INFO.RAM * 1024;
#endif

  const uint32_t ram_end = ram_start + ram_size;

  if ((uintptr_t)start_addr >= ram_start && (uintptr_t)start_addr < ram_end) {
    return TICOS_MIN(desired_size, ram_end - (uintptr_t)start_addr);
  }
  return 0;
}

#if !TICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS

  #ifndef TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY
    #define TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY 1
  #endif

  #ifndef TICOS_COREDUMP_RAM_REGION_START_ADDR
extern uint32_t __TicosCoredumpRamStart[];
    #define TICOS_COREDUMP_STORAGE_START_ADDR ((uint32_t)__TicosCoreStorageStart)
  #endif

  #ifndef TICOS_COREDUMP_RAM_REGION_END_ADDR
extern uint32_t __TcsCoredumpRamEnd[];
    #define TICOS_COREDUMP_STORAGE_END_ADDR ((uint32_t)__TicosCoreStorageEnd)
  #endif

const sTcsCoredumpRegion *ticos_platform_coredump_get_regions(
  const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
  // Let's collect the callstack at the time of crash
  static sTcsCoredumpRegion s_coredump_regions[1];

  #if (TICOS_PLATFORM_COREDUMP_CAPTURE_STACK_ONLY == 1)
  const void *stack_start_addr = crash_info->stack_address;
  // Capture only the interrupt stack. Use only if there is not enough storage to capture all of
  // RAM.
  s_coredump_regions[0] = TICOS_COREDUMP_MEMORY_REGION_INIT(
    stack_start_addr, (uintptr_t)STACK_TOP - (uintptr_t)stack_start_addr);
  #else
  // Capture all of RAM. Recommended: it enables broader post-mortem analyses,
  // but has larger storage requirements.
  s_coredump_regions[0] = TICOS_COREDUMP_MEMORY_REGION_INIT(
    TICOS_COREDUMP_RAM_REGION_START_ADDR,
    TICOS_COREDUMP_RAM_REGION_END_ADDR - TICOS_COREDUMP_RAM_REGION_START_ADDR);
  #endif

  *num_regions = TICOS_ARRAY_SIZE(s_coredump_regions);
  return s_coredump_regions;
}
#endif  // TICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS
