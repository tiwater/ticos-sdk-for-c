//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Implements "ticos_platform_coredump_get_regions()" which defines
//! the RAM regions to collect as part of a coredump. The function is defined
//! as weak so an end user can easily override and change the regions collected.
//!
//! Via Kconfig, three options are provided by default:
//!  1. Collect a minimal set of RAM to perform unwinds of all FreeRTOS tasks
//!     (CONFIG_TICOS_COREDUMP_REGIONS_THREADS_ONLY=y)
//!  2. Capture all of RAM (CONFIG_TICOS_COREDUMP_REGIONS_ALL_RAM=y) (default)
//!  3. Use custom implementation (CONFIG_TICOS_COREDUMP_REGIONS_CUSTOM=y)

#include "sdkconfig.h"

#include "ticos/panics/platform/coredump.h"
#include "ticos/ports/freertos_coredump.h"

#include <stdint.h>
#include <stddef.h>

#include "ticos/core/compiler.h"
#include "ticos/core/math.h"

//! The maximum amount of non-task regions we will track
//! (current sp, _iram_bss, _bss, _data, _iram)
#define TICOS_MAX_EXTRA_REGIONS 5

#define TICOS_MAX_REGIONS (TICOS_PLATFORM_MAX_TASK_REGIONS + TICOS_MAX_EXTRA_REGIONS)

static sTcsCoredumpRegion s_coredump_regions[TICOS_MAX_REGIONS];

typedef struct RamRegions {
  uint32_t start_addr;
  size_t length;
} sRamRegions;

// Regions are defined in linker script:
//   https://github.com/espressif/ESP8266_RTOS_SDK/blob/v3.3/components/esp8266/ld/esp8266.ld#L19-L44
//
// Unfortunately, the sizes are not externed as variables so we copy a mapping here
const sRamRegions s_valid_8266_ram_regions[] = {
  { .start_addr = 0x3FFE8000, .length = 0x18000 },
  { .start_addr = 0x40100000, .length = CONFIG_SOC_IRAM_SIZE }
};

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_valid_8266_ram_regions); i++) {
    const sRamRegions *region = &s_valid_8266_ram_regions[i];
    const uint32_t end_addr = region->start_addr + region->length;
    if ((uint32_t)start_addr >= region->start_addr && ((uint32_t)start_addr < end_addr)) {
      return TICOS_MIN(desired_size, end_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

#if !CONFIG_TICOS_COREDUMP_REGIONS_CUSTOM
const sTcsCoredumpRegion *ticos_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions) {
   int region_idx = 0;
   const size_t stack_size_to_collect =
       ticos_platform_sanitize_address_range(crash_info->stack_address, 512);
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       crash_info->stack_address, stack_size_to_collect);

   // Note: The FreeRTOS task contexts are placed in this region so we always need to
   // collect it
   extern uint32_t _iram_bss_start[];
   extern uint32_t _iram_bss_end[];
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       _iram_bss_start, (uint32_t)(_iram_bss_end) - (uint32_t)_iram_bss_start);

#if CONFIG_TICOS_COREDUMP_REGIONS_THREADS_ONLY
   region_idx += ticos_freertos_get_task_regions(
       &s_coredump_regions[region_idx], TICOS_ARRAY_SIZE(s_coredump_regions) - region_idx);

#elif CONFIG_TICOS_COREDUMP_REGIONS_ALL_RAM

   extern uint32_t _bss_start[];
   extern uint32_t _bss_end[];
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       _bss_start, (uint32_t)(_bss_end) - (uint32_t)_bss_start);

   extern uint32_t _data_start[];
   extern uint32_t _data_end[];
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       _data_start, (uint32_t)(_data_end) - (uint32_t)_data_start);


   //
   // Heaps are allocated in the remaining space at the end of RAM regions.
   // See components/heap/port/esp8266/esp_heap_init.c
   //

   const size_t bss_size = 0x40000000 - (uint32_t)_bss_end;
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       _bss_end, bss_size);

#ifndef CONFIG_HEAP_DISABLE_IRAM
   extern uint32_t _iram_end[];
   const size_t iram_size = 0x40100000 + CONFIG_SOC_IRAM_SIZE - ((size_t)_iram_end);
   s_coredump_regions[region_idx++] = TICOS_COREDUMP_MEMORY_REGION_INIT(
       _iram_end, iram_size);
#endif


#endif /* TICOS_COREDUMP_REGIONS_THREADS_ONLY */

   *num_regions = region_idx;
   return &s_coredump_regions[0];
}

#endif /* TICOS_COREDUMP_REGIONS_CUSTOM */
