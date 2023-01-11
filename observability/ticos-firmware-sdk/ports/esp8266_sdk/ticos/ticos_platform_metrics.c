//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port of dependency functions for Ticos metrics subsystem using the ESP8266 SDK.
//!
//! @note For test purposes, the heartbeat interval can be changed to a faster period
//! by using the following CFLAG:
//!   -DTICOS_METRICS_HEARTBEAT_INTERVAL_SECS=15

#include "sdkconfig.h"

#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/timer.h"

#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "FreeRTOS.h"
#include "task.h"

TICOS_WEAK
void ticos_esp_metric_timer_dispatch(TicosPlatformTimerCallback handler) {
  if (handler == NULL) {
    return;
  }

#if CONFIG_TICOS_HEARTBEAT_TRACK_HEAP_USAGE
  // We are about to service heartbeat data so get the latest stats for the
  // statistics being automatically tracked by the port
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_FreeSize),
                                          heap_caps_get_free_size(0));
  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_MinFreeSize),
                                          heap_caps_get_minimum_free_size(0));
#endif

#if CONFIG_TICOS_HEARTBEAT_TRACK_MAIN_STACK_HWM

#if INCLUDE_xTaskGetHandle == 0
 #error "Add #define INCLUDE_xTaskGetHandle 1 to FreeRTOSConfig.h"
#endif
  TaskHandle_t xHandle = xTaskGetHandle("uiT");

  ticos_metrics_heartbeat_set_unsigned(
      TICOS_METRICS_KEY(MainTask_StackHighWaterMarkWords),
      uxTaskGetStackHighWaterMark(xHandle));
#endif

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

  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

  const int64_t us_per_sec = 1000000;
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, period_sec * us_per_sec));

  return true;
}
