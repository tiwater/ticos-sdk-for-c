//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <string.h>

#include "fakes/fake_ticos_platform_metrics_locking.h"
#include "ticos/components.h"
#include "ticos/metrics/serializer.h"

extern "C" {
  uint64_t ticos_platform_get_time_since_boot_ms(void) {
    return 0;
  }
}

bool ticos_platform_metrics_timer_boot(uint32_t period_sec,
                                          TICOS_UNUSED TicosPlatformTimerCallback callback) {
  return mock().actualCall(__func__)
      .withParameter("period_sec", period_sec)
      .returnBoolValueOrDefault(true);
}

size_t ticos_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(0);
}

bool ticos_metrics_heartbeat_serialize(
    TICOS_UNUSED const sTicosEventStorageImpl *storage_impl) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(true);
}

TEST_GROUP(TicosHeartbeatMetricsNoCustom){
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

//! Confirm compilation and metric count is correct
TEST(TicosHeartbeatMetricsNoCustom, Test_CompileAndMetricCount) {
  size_t num_metrics = ticos_metrics_heartbeat_get_num_metrics();
  LONGS_EQUAL(3, num_metrics);
}

//! Confirm we can boot without any issues (eg writing to unpopulated key
//! position)
TEST(TicosHeartbeatMetricsNoCustom, Test_Boot) {

  // Check that by default the heartbeat interval is once / hour
  mock().expectOneCall("ticos_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  mock().expectOneCall("ticos_metrics_heartbeat_compute_worst_case_storage_size")
      .andReturnValue(0);

  // Mock an initial reboot reason for initial metric setup
  bool unexpected_reboot = true;
  mock()
    .expectOneCall("ticos_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);

  static uint8_t s_storage[1000];

  static const sTicosEventStorageImpl *fake_event_storage_impl = ticos_events_storage_boot(
      &s_storage, sizeof(s_storage));

  sTicosMetricBootInfo boot_info = { .unexpected_reboot_count = 1 };
  int rv = ticos_metrics_boot(fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(0, rv);

  // call this to get coverage on the internal weak function
  ticos_metrics_heartbeat_collect_data();
}
