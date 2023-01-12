//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/core/heap_stats.h"
#include "ticos/core/heap_stats_impl.h"
#include "ticos/core/log_impl.h"
#include "ticos/panics/coredump_impl.h"

// fakes for populating heap_stats in coredump regions
sTcsHeapStats g_ticos_heap_stats;
sTcsHeapStatEntry g_ticos_heap_stats_pool[TICOS_HEAP_STATS_MAX_COUNT];

TEST_GROUP(TicosSdkRegions){
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

bool ticos_log_get_regions(sTicosLogRegions *regions) {
  const bool enabled = mock().actualCall(__func__).returnBoolValueOrDefault(true);
  if (!enabled) {
    return false;
  }

  for (size_t i = 0; i < TICOS_LOG_NUM_RAM_REGIONS; i++) {
    sTicosLogMemoryRegion *region = &regions->region[i];
    region->region_start = (void *)i;
    region->region_size = i + 1;
  }
  return true;
}

bool ticos_heap_stats_empty(void) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(false);
}

static void prv_check_heap_stats_region(const sTcsCoredumpRegion *region) {
  LONGS_EQUAL(kTcsCoredumpRegionType_Memory, region->type);
  LONGS_EQUAL((uintptr_t)&g_ticos_heap_stats, (uintptr_t)region->region_start);
  LONGS_EQUAL(sizeof(g_ticos_heap_stats), (uint32_t)region->region_size);
}

static void prv_check_heap_stats_pool_region(const sTcsCoredumpRegion *region) {
  LONGS_EQUAL(kTcsCoredumpRegionType_Memory, region->type);
  LONGS_EQUAL((uintptr_t)&g_ticos_heap_stats_pool, (uintptr_t)region->region_start);
  LONGS_EQUAL(sizeof(g_ticos_heap_stats_pool), (uint32_t)region->region_size);
}


TEST(TicosSdkRegions, Test_TicosSdkRegions_Enabled) {

  mock().expectOneCall("ticos_log_get_regions");
  mock().expectOneCall("ticos_heap_stats_empty");
  size_t num_regions;
  const sTcsCoredumpRegion *regions = ticos_coredump_get_sdk_regions(&num_regions);
  LONGS_EQUAL(4, num_regions);

  const sTcsCoredumpRegion *region = nullptr;
  for (size_t i = 0; i < TICOS_LOG_NUM_RAM_REGIONS; i++) {
    region = &regions[i];
    LONGS_EQUAL(kTcsCoredumpRegionType_Memory, region->type);
    LONGS_EQUAL(i, (uint32_t)(uintptr_t)region->region_start);
    LONGS_EQUAL(i + 1, (uint32_t)region->region_size);
  }

  region++;
  prv_check_heap_stats_region(region);
  region++;
  prv_check_heap_stats_pool_region(region);
}

TEST(TicosSdkRegions, Test_TicosSdkRegions_Disabled) {
  mock().expectOneCall("ticos_log_get_regions").andReturnValue(false);
  mock().expectOneCall("ticos_heap_stats_empty").andReturnValue(true);
  size_t num_regions;
  const sTcsCoredumpRegion *regions = ticos_coredump_get_sdk_regions(&num_regions);
  LONGS_EQUAL(0, num_regions);
  CHECK(regions == NULL);
}
