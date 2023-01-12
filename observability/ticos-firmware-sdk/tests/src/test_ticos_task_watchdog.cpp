//! @file
//!
//! @brief

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "comparators/comparator_ticos_fault_handling.hpp"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/task_watchdog.h"

extern "C" {
static uint64_t s_fake_time_ms = 0;
uint64_t ticos_platform_get_time_since_boot_ms(void) { return s_fake_time_ms; }
}

void ticos_task_watchdog_platform_refresh_callback(void) { mock().actualCall(__func__); }

static Tcs_sTicosAssertInfo_Comparator s_assert_info_comparator;

TEST_GROUP(TicosTaskWatchdog){void setup(){mock().strictOrder();
mock().installComparator("sTicosAssertInfo", s_assert_info_comparator);

s_fake_time_ms = 0;

ticos_task_watchdog_init();
}
void teardown() {
  mock().checkExpectations();
  mock().removeAllComparatorsAndCopiers();
  mock().clear();
}
}
;

TEST(TicosTaskWatchdog, Test_Basic) {
  // no channels started
  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();

  // start a channel, confirm we don't expire when time hasn't advanced
  TICOS_TASK_WATCHDOG_START(task_1);
  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();

  // call for coverage
  ticos_task_watchdog_bookkeep();
}

TEST(TicosTaskWatchdog, Test_Expire) {
  // start a channel and artificially advance time to expire it
  TICOS_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS - 1;
  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();

  s_fake_time_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;
  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();

  s_fake_time_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;

  sTicosAssertInfo extra_info = {
    .assert_reason = kTcsRebootReason_SoftwareWatchdog,
  };
  mock()
    .expectOneCall("ticos_fault_handling_assert_extra")
    .withPointerParameter("pc", 0)
    .withPointerParameter("lr", 0)
    .withParameterOfType("sTicosAssertInfo", "extra_info", &extra_info);

  ticos_task_watchdog_check_all();
}

TEST(TicosTaskWatchdog, Test_Feed) {
  // start a channel, advance time past the timeout, feed it, confirm it doesn't
  // expire
  TICOS_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;
  TICOS_TASK_WATCHDOG_FEED(task_1);

  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();
}

TEST(TicosTaskWatchdog, Test_Stop) {
  // start a channel, advance time past the timeout, feed it, confirm it doesn't
  // expire
  TICOS_TASK_WATCHDOG_START(task_1);

  s_fake_time_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS + 1;
  TICOS_TASK_WATCHDOG_STOP(task_1);

  mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
  ticos_task_watchdog_check_all();
}

TEST(TicosTaskWatchdog, Test_ExpireWrapAround) {
  // start a channel and artificially advance time to expire it. check
  // the wraparound cases.
  const uint64_t wrap_start_points[] = {
    UINT32_MAX,
    UINT64_MAX,
  };

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(wrap_start_points); i++){
    s_fake_time_ms = wrap_start_points[i] - TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;
    TICOS_TASK_WATCHDOG_START(task_1);

    s_fake_time_ms += TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS - 1;
    mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
    ticos_task_watchdog_check_all();

    s_fake_time_ms += 1;
    mock().expectOneCall("ticos_task_watchdog_platform_refresh_callback");
    ticos_task_watchdog_check_all();

    s_fake_time_ms += 1;

    sTicosAssertInfo extra_info = {
      .assert_reason = kTcsRebootReason_SoftwareWatchdog,
    };
    mock()
      .expectOneCall("ticos_fault_handling_assert_extra")
      .withPointerParameter("pc", 0)
      .withPointerParameter("lr", 0)
      .withParameterOfType("sTicosAssertInfo", "extra_info", &extra_info);

    ticos_task_watchdog_check_all();
  }
}
