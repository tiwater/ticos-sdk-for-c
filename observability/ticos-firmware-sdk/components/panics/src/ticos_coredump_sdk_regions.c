//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Enables collection of memory regions used by the Ticos SDK based on compile time defines.
//!
//! This is intended for users that are collecting select RAM regions in a coredump
//! (instead of all of RAM).
//!
//! Regions to collect are controlled by the following compile time defines:
//!
//!  TICOS_COREDUMP_COLLECT_LOG_REGIONS=1
//!   - Collects the regions used by the Ticos logging APIs. When enabled,
//!     any logs collected will be included in the coredump. More details
//!     can be found at https://ticos.io/logging
//!  TICOS_COREDUMP_COLLECT_HEAP_STATS=1
//!   - Collects the regions used by the Ticos heap stat APIs. When enabled,
//!     statistics around heap allocations will be included in the coredump to facilitate
//!     debug

#include "ticos/config.h"
#include "ticos/core/heap_stats_impl.h"
#include "ticos/core/log_impl.h"
#include "ticos/core/math.h"
#include "ticos/core/task_watchdog_impl.h"
#include "ticos/panics/coredump_impl.h"

// The number of regions to be collected varies per configuration

// clang-format off
#define CONFIGURED_NUM_REGIONS_TO_COLLECT (0 \
  + (TICOS_COREDUMP_COLLECT_LOG_REGIONS ? TICOS_LOG_NUM_RAM_REGIONS : 0) \
  + (TICOS_COREDUMP_COLLECT_HEAP_STATS ? TICOS_HEAP_STATS_NUM_RAM_REGIONS : 0) \
  + (TICOS_COREDUMP_COLLECT_TASK_WATCHDOG_REGION ? 1 : 0)\
)
// clang-format on

// if no special sdk regions are enabled, default array size to 1 to silence
// compiler warnings
#define NUM_REGIONS_TO_COLLECT TICOS_MAX(CONFIGURED_NUM_REGIONS_TO_COLLECT, 1)

const sTcsCoredumpRegion *ticos_coredump_get_sdk_regions(size_t *num_regions) {
  static sTcsCoredumpRegion s_sdk_coredump_regions[NUM_REGIONS_TO_COLLECT];

  size_t total_regions = 0;

#if TICOS_COREDUMP_COLLECT_LOG_REGIONS
  sTicosLogRegions regions = {0};
  if (ticos_log_get_regions(&regions)) {
    for (size_t i = 0; i < TICOS_LOG_NUM_RAM_REGIONS; i++) {
      sTicosLogMemoryRegion *log_region = &regions.region[i];
      s_sdk_coredump_regions[total_regions] =
        TICOS_COREDUMP_MEMORY_REGION_INIT(log_region->region_start, log_region->region_size);
      total_regions++;
    }
  }
#endif

#if TICOS_COREDUMP_COLLECT_HEAP_STATS
  if (!ticos_heap_stats_empty()) {
    s_sdk_coredump_regions[total_regions] =
      TICOS_COREDUMP_MEMORY_REGION_INIT(&g_ticos_heap_stats, sizeof(g_ticos_heap_stats));
    total_regions++;
    s_sdk_coredump_regions[total_regions] = TICOS_COREDUMP_MEMORY_REGION_INIT(
      &g_ticos_heap_stats_pool, sizeof(g_ticos_heap_stats_pool));
    total_regions++;
  }
#endif

#if TICOS_COREDUMP_COLLECT_TASK_WATCHDOG_REGION
  s_sdk_coredump_regions[total_regions] = TICOS_COREDUMP_MEMORY_REGION_INIT(
    &g_ticos_task_channel_info, sizeof(g_ticos_task_channel_info));
  total_regions++;
#endif

  *num_regions = total_regions;
  return total_regions != 0 ? &s_sdk_coredump_regions[0] : NULL;
}
