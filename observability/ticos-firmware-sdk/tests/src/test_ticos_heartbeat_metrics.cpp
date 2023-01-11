//! @file
//!
//! @brief

#include <stddef.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_ticos_platform_metrics_locking.h"
#include "ticos/core/compiler.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/overrides.h"
#include "ticos/metrics/platform/timer.h"
#include "ticos/metrics/serializer.h"
#include "ticos/metrics/utils.h"

extern "C" {
  static void (*s_serializer_check_cb)(void) = NULL;

  static uint64_t s_fake_time_ms = 0;
  uint64_t ticos_platform_get_time_since_boot_ms(void) {
    return s_fake_time_ms;
  }

  static void prv_fake_time_set(uint64_t new_fake_time_ms) {
    s_fake_time_ms = new_fake_time_ms;
  }

  static void prv_fake_time_incr(uint64_t fake_time_delta_ms) {
    s_fake_time_ms += fake_time_delta_ms;
  }

  static const sTicosEventStorageImpl *s_fake_event_storage_impl;
}

bool ticos_platform_metrics_timer_boot(uint32_t period_sec,
                                          TICOS_UNUSED TicosPlatformTimerCallback callback) {
  return mock().actualCall(__func__)
      .withParameter("period_sec", period_sec)
      .returnBoolValueOrDefault(true);
}


#define FAKE_STORAGE_SIZE 100

bool ticos_metrics_heartbeat_serialize(
    TICOS_UNUSED const sTicosEventStorageImpl *storage_impl) {
  mock().actualCall(__func__);
  if (s_serializer_check_cb != NULL) {
    s_serializer_check_cb();
  }

  return true;
}

size_t ticos_metrics_heartbeat_compute_worst_case_storage_size(void) {
  return (size_t)mock().actualCall(__func__).returnIntValueOrDefault(FAKE_STORAGE_SIZE);
}

TEST_GROUP(TicosHeartbeatMetrics){
  void setup() {
    s_fake_time_ms = 0;
    s_serializer_check_cb = NULL;
    fake_ticos_metrics_platorm_locking_reboot();
    static uint8_t s_storage[FAKE_STORAGE_SIZE];
    mock().strictOrder();

    // Check that by default the heartbeat interval is once / hour
    mock().expectOneCall("ticos_platform_metrics_timer_boot").withParameter("period_sec", 3600);

    s_fake_event_storage_impl = ticos_events_storage_boot(
        &s_storage, sizeof(s_storage));
    mock().expectOneCall("ticos_metrics_heartbeat_compute_worst_case_storage_size");

    // Mock an initial reboot reason for initial metric setup
    bool unexpected_reboot = true;
    mock()
      .expectOneCall("ticos_reboot_tracking_get_unexpected_reboot_occurred")
      .withOutputParameterReturning("unexpected_reboot_occurred", &unexpected_reboot,
                                    sizeof(unexpected_reboot))
      .andReturnValue(0);

    sTicosMetricBootInfo boot_info = { .unexpected_reboot_count = 7 };
    int rv = ticos_metrics_boot(s_fake_event_storage_impl, &boot_info);
    LONGS_EQUAL(0, rv);
    mock().checkExpectations();

    // crash count should have beem copied into heartbeat
    uint32_t logged_crash_count = 0;
    ticos_metrics_heartbeat_read_unsigned(
        TICOS_METRICS_KEY(TicosSdkMetric_UnexpectedRebootCount), &logged_crash_count);
    LONGS_EQUAL(boot_info.unexpected_reboot_count, logged_crash_count);

    uint32_t logged_reboot_did_occur = 0;
    ticos_metrics_heartbeat_read_unsigned(
      TICOS_METRICS_KEY(TicosSdkMetric_UnexpectedRebootDidOccur), &logged_reboot_did_occur);
    LONGS_EQUAL(1, logged_reboot_did_occur);

    // IntervalMs & RebootCount & ResetDidOccur
    const size_t num_ticos_sdk_metrics = 3;

    // We should test all the types of available metrics so if this
    // fails it means there's a new type we aren't yet covering
    LONGS_EQUAL(kTicosMetricType_NumTypes + num_ticos_sdk_metrics,
                ticos_metrics_heartbeat_get_num_metrics());
  }
  void teardown() {
    // dump the final result & also sanity test that this routine works
    ticos_metrics_heartbeat_debug_print();
    CHECK(fake_ticos_platform_metrics_lock_calls_balanced());
    // we are simulating a reboot, so the timer should be running.
    // Let's stop it and confirm that works and reset our state so the next
    // boot call will succeed.
    TicosMetricId id = TICOS_METRICS_KEY(TicosSdkMetric_IntervalMs);
    int rv = ticos_metrics_heartbeat_timer_stop(id);
    LONGS_EQUAL(0, rv);
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(TicosHeartbeatMetrics, Test_BootStorageTooSmall) {
  // Check that by default the heartbeat interval is once / hour
  mock().expectOneCall("ticos_platform_metrics_timer_boot").withParameter("period_sec", 3600);

  // reboot metrics with storage that is too small to actually hold an event
  // this should result in a warning being emitted
  mock().expectOneCall("ticos_metrics_heartbeat_compute_worst_case_storage_size")
      .andReturnValue(FAKE_STORAGE_SIZE + 1);

  sTicosMetricBootInfo boot_info = { .unexpected_reboot_count = 1 };
  int rv = ticos_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(-5, rv);
  mock().checkExpectations();
}

TEST(TicosHeartbeatMetrics, Test_TimerInitFailed) {
  // Nothing else should happen if timer initialization failed for some reason
  mock().expectOneCall("ticos_platform_metrics_timer_boot")
      .withParameter("period_sec", 3600)
      .andReturnValue(false);

  sTicosMetricBootInfo boot_info = { .unexpected_reboot_count = 1 };
  int rv = ticos_metrics_boot(s_fake_event_storage_impl, &boot_info);
  LONGS_EQUAL(-6, rv);
  mock().checkExpectations();
}

TEST(TicosHeartbeatMetrics, Test_UnsignedHeartbeatValue) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_unsigned);
  int rv = ticos_metrics_heartbeat_set_unsigned(key, 100);
  LONGS_EQUAL(0, rv);

  rv = ticos_metrics_heartbeat_set_signed(key, 100);
  CHECK(rv != 0);

  rv = ticos_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = ticos_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = ticos_metrics_heartbeat_add(key, 2);
  LONGS_EQUAL(0, rv);
  uint32_t val = 0;
  rv = ticos_metrics_heartbeat_read_unsigned(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(104, val);

  // test clipping
  ticos_metrics_heartbeat_add(key, INT32_MAX);
  ticos_metrics_heartbeat_add(key, INT32_MAX);
  rv = ticos_metrics_heartbeat_read_unsigned(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(UINT32_MAX, val);
}

TEST(TicosHeartbeatMetrics, Test_SignedHeartbeatValue) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_signed);
  int rv = ticos_metrics_heartbeat_set_signed(key, -100);
  LONGS_EQUAL(0, rv);

  // try wrong types
  rv = ticos_metrics_heartbeat_set_unsigned(key, 100);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  rv = ticos_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = ticos_metrics_heartbeat_add(key, 1);
  LONGS_EQUAL(0, rv);
  rv = ticos_metrics_heartbeat_add(key, 2);
  LONGS_EQUAL(0, rv);
  int32_t val = 0;
  rv = ticos_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(-96, val);

  ticos_metrics_heartbeat_add(key, INT32_MAX);
  ticos_metrics_heartbeat_add(key, INT32_MAX);
  rv = ticos_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(INT32_MAX, val);

  ticos_metrics_heartbeat_set_signed(key, -100);
  ticos_metrics_heartbeat_add(key, INT32_MIN);
  rv = ticos_metrics_heartbeat_read_signed(key, &val);
  LONGS_EQUAL(0, rv);
  LONGS_EQUAL(INT32_MIN, val);
}

TEST(TicosHeartbeatMetrics, Test_TimerHeartBeatValueSimple) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_timer);
  // no-op
  int rv = ticos_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  // start the timer
  rv = ticos_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_incr(10);
  // no-op
  rv = ticos_metrics_heartbeat_timer_start(key);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_add(key, 20);
  CHECK(rv != 0);

  rv = ticos_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(10, val);
}

TEST(TicosHeartbeatMetrics, Test_TimerHeartBeatValueRollover) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_timer);

  prv_fake_time_set(0x80000000 - 9);

  int rv = ticos_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_set(0x80000008);

  rv = ticos_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(17, val);
}

TEST(TicosHeartbeatMetrics, Test_TimerHeartBeatNoChange) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_timer);

  int rv = ticos_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);


  rv = ticos_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);

  uint32_t val;
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);
}

#define EXPECTED_HEARTBEAT_TIMER_VAL_MS  13

static void prv_serialize_check_cb(void) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_timer);
  uint32_t val;
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(EXPECTED_HEARTBEAT_TIMER_VAL_MS, val);
}

TEST(TicosHeartbeatMetrics, Test_TimerActiveWhenHeartbeatCollected) {
  TicosMetricId key = TICOS_METRICS_KEY(test_key_timer);

  int rv = ticos_metrics_heartbeat_timer_start(key);
  LONGS_EQUAL(0, rv);
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);

  s_serializer_check_cb = &prv_serialize_check_cb;
  mock().expectOneCall("ticos_metrics_heartbeat_collect_data");
  mock().expectOneCall("ticos_metrics_heartbeat_serialize");
  ticos_metrics_heartbeat_debug_trigger();

  uint32_t val;
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);

  // timer should still be running
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);
  rv = ticos_metrics_heartbeat_timer_stop(key);
  LONGS_EQUAL(0, rv);
  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(EXPECTED_HEARTBEAT_TIMER_VAL_MS, val);

  // the time is no longer running so an increment shouldn't be counted
  prv_fake_time_incr(EXPECTED_HEARTBEAT_TIMER_VAL_MS);
  s_serializer_check_cb = &prv_serialize_check_cb;
  mock().expectOneCall("ticos_metrics_heartbeat_collect_data");
  mock().expectOneCall("ticos_metrics_heartbeat_serialize");
  ticos_metrics_heartbeat_debug_trigger();

  ticos_metrics_heartbeat_timer_read(key, &val);
  LONGS_EQUAL(0, val);
}

TEST(TicosHeartbeatMetrics, Test_String) {
  #define SAMPLE_STRING "0123456789abcdef"
  static_assert(__builtin_strlen(SAMPLE_STRING) == 16,
                "be sure to modify tests/stub_includes/ticos_metrics_heartbeat_config.def to "
                "match exactly, so we can check for buffer overflows!");

  TicosMetricId key = TICOS_METRICS_KEY(test_key_string);

  // just the correct size
  {
    int rv = ticos_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) + 1];
    memset(sample_string, 0, sizeof(sample_string));

    rv = ticos_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SAMPLE_STRING, (const char *)sample_string);
  }

  // set too long a string
  {
    int rv = ticos_metrics_heartbeat_set_string(key, SAMPLE_STRING "1");
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) + 1];
    memset(sample_string, 0, sizeof(sample_string));

    rv = ticos_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SAMPLE_STRING, (const char *)sample_string);
  }

  // read with bad destination buffer
  {
    int rv = ticos_metrics_heartbeat_read_string(key, NULL, 0);
    CHECK(rv != 0);
  }

  // write with longer then shorter string and confirm readback is ok
  {
    int rv = ticos_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);
    #define SHORT_TEST_STRING "12"
    rv = ticos_metrics_heartbeat_set_string(key, SHORT_TEST_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SHORT_TEST_STRING)];
    memset(sample_string, 0, sizeof(sample_string));

    rv = ticos_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL(SHORT_TEST_STRING, (const char *)sample_string);
  }

  // read with a buffer that's too small, and confirm it's a valid string
  {
    int rv = ticos_metrics_heartbeat_set_string(key, SAMPLE_STRING);
    LONGS_EQUAL(0, rv);

    char sample_string[sizeof(SAMPLE_STRING) - 1];
    memset(sample_string, 'a', sizeof(sample_string));

    rv = ticos_metrics_heartbeat_read_string(key, sample_string, sizeof(sample_string));
    LONGS_EQUAL(0, rv);
    STRCMP_EQUAL("0123456789abcde", (const char *)sample_string);
  }


}

TEST(TicosHeartbeatMetrics, Test_BadBoot) {
  sTicosMetricBootInfo info = { .unexpected_reboot_count = 1 };

  int rv = ticos_metrics_boot(NULL, &info);
  LONGS_EQUAL(-3, rv);

  rv = ticos_metrics_boot(s_fake_event_storage_impl, NULL);
  LONGS_EQUAL(-3, rv);

  rv = ticos_metrics_boot(NULL, NULL);
  LONGS_EQUAL(-3, rv);

  // calling boot with valid args twice in a row should fail (interval timer already running)
  mock().expectOneCall("ticos_platform_metrics_timer_boot").withParameter("period_sec", 3600);
  mock().expectOneCall("ticos_metrics_heartbeat_compute_worst_case_storage_size");
  rv = ticos_metrics_boot(s_fake_event_storage_impl, &info);
  LONGS_EQUAL(-4, rv);
}

TEST(TicosHeartbeatMetrics, Test_KeyDNE) {
  // NOTE: Using the macro TICOS_METRICS_KEY, it's impossible for a non-existent key to trigger a
  // compilation error so we just create an invalid key.
  TicosMetricId key = (TicosMetricId){ INT32_MAX };

  int rv = ticos_metrics_heartbeat_set_signed(key, 0);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_set_unsigned(key, 0);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_add(key, INT32_MIN);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_set_string(key, NULL);
  CHECK(rv != 0);

  rv = ticos_metrics_heartbeat_timer_start(key);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_timer_stop(key);
  CHECK(rv != 0);

  int32_t vali32;
  rv = ticos_metrics_heartbeat_read_signed(key, NULL);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_read_signed(key, &vali32);
  CHECK(rv != 0);

  uint32_t valu32;
  rv = ticos_metrics_heartbeat_read_unsigned(key, NULL);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_read_unsigned(key, &valu32);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_timer_read(key, NULL);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_timer_read(key, &valu32);
  CHECK(rv != 0);
  rv = ticos_metrics_heartbeat_read_string(key, NULL, 0);
  CHECK(rv != 0);
}

void ticos_metrics_heartbeat_collect_data(void) {
  mock().actualCall(__func__);
}

TEST(TicosHeartbeatMetrics, Test_HeartbeatCollection) {
  TicosMetricId keyi32 = TICOS_METRICS_KEY(test_key_signed);
  TicosMetricId keyu32 = TICOS_METRICS_KEY(test_key_unsigned);

  ticos_metrics_heartbeat_set_signed(keyi32, 200);
  ticos_metrics_heartbeat_set_unsigned(keyu32, 199);

  int32_t vali32;
  uint32_t valu32;

  // should fail if we read the wrong type
  int rv = ticos_metrics_heartbeat_read_signed(keyu32, &vali32);
  CHECK(rv != 0);

  rv = ticos_metrics_heartbeat_read_signed(keyi32, &vali32);
  LONGS_EQUAL(0, rv);
  rv = ticos_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(0, rv);

  LONGS_EQUAL(vali32, 200);
  LONGS_EQUAL(valu32, 199);

  mock().expectOneCall("ticos_metrics_heartbeat_collect_data");
  mock().expectOneCall("ticos_metrics_heartbeat_serialize");
  ticos_metrics_heartbeat_debug_trigger();
  mock().checkExpectations();

  // values should all be reset
  ticos_metrics_heartbeat_read_signed(keyi32, &vali32);
  ticos_metrics_heartbeat_read_unsigned(keyu32, &valu32);
  LONGS_EQUAL(vali32, 0);
  LONGS_EQUAL(valu32, 0);
}
