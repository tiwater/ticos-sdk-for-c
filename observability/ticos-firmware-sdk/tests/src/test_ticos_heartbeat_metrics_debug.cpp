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
  static uint64_t s_boot_time_ms = 0;

  uint64_t ticos_platform_get_time_since_boot_ms(void) {
    return s_boot_time_ms;
  }
}

bool ticos_platform_metrics_timer_boot(uint32_t period_sec,
                                          TICOS_UNUSED TicosPlatformTimerCallback callback) {
  return mock().actualCall(__func__)
      .withParameter("period_sec", period_sec)
      .returnBoolValueOrDefault(true);
}

bool ticos_metrics_heartbeat_serialize(
    TICOS_UNUSED const sTicosEventStorageImpl *storage_impl) {
  return mock().actualCall(__func__)
      .returnBoolValueOrDefault(true);
}

size_t ticos_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(0);
}

TEST_GROUP(TicosHeartbeatMetricsDebug){
  void setup() {
    s_boot_time_ms = 0;
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_debug_print_expectations(eTicosPlatformLogLevel level, const char *const lines[],
                                         size_t num_lines) {
  for (unsigned int i = 0; i < num_lines; ++i) {
    mock()
      .expectOneCall("ticos_platform_log")
      .withIntParameter("level", level)
      .withStringParameter("output", lines[i]);
  }
}

void ticos_metrics_heartbeat_collect_data(void) {
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(test_key_unsigned), 1234);
  ticos_metrics_heartbeat_set_signed(TICOS_METRICS_KEY(test_key_signed), -100);
  ticos_metrics_heartbeat_set_string(TICOS_METRICS_KEY(test_key_string), "heyo!");

  // add a call here to ensure it doesn't recurse endlessly. customers may be
  // using this pattern
  ticos_metrics_heartbeat_debug_print();
}

//! check the debug print outputs the correct values depending on metrics state
TEST(TicosHeartbeatMetricsDebug, Test_DebugPrints) {
  static uint8_t s_storage[1000];

  mock().expectOneCall("ticos_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  const sTicosEventStorageImpl *s_fake_event_storage_impl =
    ticos_events_storage_boot(&s_storage, sizeof(s_storage));
  mock().expectOneCall("ticos_metrics_heartbeat_compute_worst_case_storage_size");

  // Mock an initial reboot reason for initial metric setup, use UnknownError to observe metric is
  // reset after collection
  // The metric TicosSdkMetric_UnexpectedRebootDidOccur should start as 1 to indicate an error,
  // Still show as 1 before the metric has been reset by the heartbeat
  // Finally show as 0 at next heartbeat
  bool unexpected_reboot = true;
  mock()
    .expectOneCall("ticos_reboot_tracking_get_unexpected_reboot_occurred")
    .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                  sizeof(unexpected_reboot))
    .andReturnValue(0);

  sTicosMetricBootInfo boot_info = {.unexpected_reboot_count = 1};
  int rv = ticos_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(0, rv);
  mock().checkExpectations();


  // this should output the system reset values
  const char *heartbeat_debug_print_on_boot[] = {
    "Heartbeat keys/values:",
    "  TicosSdkMetric_IntervalMs: 0",
    "  TicosSdkMetric_UnexpectedRebootCount: 1",
    "  TicosSdkMetric_UnexpectedRebootDidOccur: 1",
    "  test_key_unsigned: 0",
    "  test_key_signed: 0",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
  };
  prv_debug_print_expectations(kTicosPlatformLogLevel_Debug, heartbeat_debug_print_on_boot,
                               TICOS_ARRAY_SIZE(heartbeat_debug_print_on_boot));
  ticos_metrics_heartbeat_debug_print();
  mock().checkExpectations();

  s_boot_time_ms = 678;
  ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(test_key_timer));
  s_boot_time_ms = 5678;

  // debug trigger will update, save, and zero the values
  const char *heartbeat_debug_print_after_collected[] = {
    "Heartbeat keys/values:",
    "  TicosSdkMetric_IntervalMs: 5678",
    "  TicosSdkMetric_UnexpectedRebootCount: 1",
    "  TicosSdkMetric_UnexpectedRebootDidOccur: 1",
    "  test_key_unsigned: 1234",
    "  test_key_signed: -100",
    "  test_key_timer: 5000",
    "  test_key_string: \"heyo!\"",
  };
  prv_debug_print_expectations(kTicosPlatformLogLevel_Debug, heartbeat_debug_print_after_collected,
                               TICOS_ARRAY_SIZE(heartbeat_debug_print_after_collected));
  mock().expectOneCall("ticos_metrics_heartbeat_serialize");
  ticos_metrics_heartbeat_debug_trigger();
  mock().checkExpectations();

  // after trigger, metrics should be zeroed now
  const char *heartbeat_debug_print_reset[] = {
    "Heartbeat keys/values:",
    "  TicosSdkMetric_IntervalMs: 0",
    "  TicosSdkMetric_UnexpectedRebootCount: 0",
    "  TicosSdkMetric_UnexpectedRebootDidOccur: 0",
    "  test_key_unsigned: 0",
    "  test_key_signed: 0",
    "  test_key_timer: 0",
    "  test_key_string: \"\"",
  };
  prv_debug_print_expectations(kTicosPlatformLogLevel_Debug, heartbeat_debug_print_reset,
                               TICOS_ARRAY_SIZE(heartbeat_debug_print_reset));
  ticos_metrics_heartbeat_debug_print();
  mock().checkExpectations();
}
