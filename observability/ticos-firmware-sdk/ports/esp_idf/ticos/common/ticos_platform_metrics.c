//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port of dependency functions for Ticos metrics subsystem using FreeRTOS.
//!
//! @note For test purposes, the heartbeat interval can be changed to a faster period
//! by using the following CFLAG:
//!   -DTICOS_METRICS_HEARTBEAT_INTERVAL_SECS=15

#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/esp_port/metrics.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/timer.h"

#include "esp_timer.h"

TICOS_WEAK
void ticos_esp_metric_timer_dispatch(TicosPlatformTimerCallback handler) {
  if (handler == NULL) {
    return;
  }
  handler();
}

static void prv_metric_timer_handler(void *arg) {
  ticos_reboot_tracking_reset_crash_count();

  // NOTE: This timer runs once per TICOS_METRICS_HEARTBEAT_INTERVAL_SECS where the default is
  // once per hour.
  TicosPlatformTimerCallback *metric_timer_handler = (TicosPlatformTimerCallback *)arg;
  ticos_esp_metric_timer_dispatch(metric_timer_handler);
}

bool ticos_platform_metrics_timer_boot(uint32_t period_sec,
                                          TicosPlatformTimerCallback callback) {
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &prv_metric_timer_handler,
    .arg = callback,
    .name = "tcs"
  };

  // Ignore return value; this function should be safe to call multiple times
  // during system init, but needs to called before we create any timers.
  // See implementation here (may change by esp-idf version!):
  // https://github.com/espressif/esp-idf/blob/master/components/esp_timer/src/esp_timer.c#L431-L460
  (void)esp_timer_init();

  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

  const int64_t us_per_sec = 1000000;
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, period_sec * us_per_sec));

  return true;
}
