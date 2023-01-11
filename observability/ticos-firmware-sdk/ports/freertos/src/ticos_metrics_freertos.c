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

#include "ticos/ports/freertos.h"

#include "ticos/core/compiler.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/timer.h"

#include "FreeRTOS.h"
#include "timers.h"

static TicosPlatformTimerCallback *s_metric_timer_cb = NULL;
static void prv_metric_timer_callback(TICOS_UNUSED TimerHandle_t handle) {
  s_metric_timer_cb();
}

static TimerHandle_t prv_metric_timer_init(const char *const pcTimerName,
                                           TickType_t xTimerPeriodInTicks,
                                           UBaseType_t uxAutoReload,
                                           void * pvTimerID,
                                           TimerCallbackFunction_t pxCallbackFunction) {
#if TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticTimer_t s_metric_timer_context;
  return xTimerCreateStatic(pcTimerName, xTimerPeriodInTicks, uxAutoReload,
                            pvTimerID, pxCallbackFunction, &s_metric_timer_context);
#else
  return xTimerCreate(pcTimerName, xTimerPeriodInTicks, uxAutoReload,
                      pvTimerID, pxCallbackFunction);
#endif
}

bool ticos_platform_metrics_timer_boot(uint32_t period_sec,
                                          TicosPlatformTimerCallback callback) {
  TimerHandle_t timer = prv_metric_timer_init("metric_timer",
                                              pdMS_TO_TICKS(period_sec * 1000),
                                              pdTRUE, /* auto reload */
                                              (void*)NULL,
                                              prv_metric_timer_callback);
  if (timer == 0) {
    return false;
  }

  s_metric_timer_cb = callback;

  xTimerStart(timer, 0);
  return true;
}
