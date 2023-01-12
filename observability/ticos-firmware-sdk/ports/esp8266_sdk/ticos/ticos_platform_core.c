//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Entry point for initialization of Ticos SDK.

#include "ticos/esp8266_port/core.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/build_info.h"
#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/core/trace_event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/semphr.h"

#include "esp_timer.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"

#ifndef TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

static const char *TAG __attribute__((unused)) = "tcs";

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      ESP_LOGD(TAG, "%s", log_buf);
      break;

    case kTicosPlatformLogLevel_Info:
      ESP_LOGI(TAG, "%s", log_buf);
      break;

    case kTicosPlatformLogLevel_Warning:
      ESP_LOGW(TAG, "%s", log_buf);
      break;

    case kTicosPlatformLogLevel_Error:
      ESP_LOGE(TAG, "%s", log_buf);
      break;
    default:
      break;
  }

  va_end(args);
}

void ticos_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  vprintf(fmt, args);
  printf("\n");

  va_end(args);
}

static SemaphoreHandle_t s_ticos_lock;

void ticos_lock(void) {
  xSemaphoreTakeRecursive(s_ticos_lock, portMAX_DELAY);
}

void ticos_unlock(void) {
  xSemaphoreGiveRecursive(s_ticos_lock);
}

uint64_t ticos_platform_get_time_since_boot_ms(void) {
  const int64_t time_since_boot_us = esp_timer_get_time();
  return  (uint64_t) (time_since_boot_us / 1000) /* us per ms */;
}

bool ticos_arch_is_inside_isr(void) {
  return xPortInIsrContext();
}

TICOS_WEAK
void ticos_platform_halt_if_debugging(void) { }

TICOS_WEAK
void ticos_esp_port_boot(void) {
  s_ticos_lock = xSemaphoreCreateRecursiveMutex();

  ticos_build_info_dump();
  ticos_esp_port_coredump_storage_boot();

#if CONFIG_TICOS_EVENT_COLLECTION_ENABLED
  ticos_esp_port_event_collection_boot();
#endif

#if CONFIG_TICOS_CLI_ENABLED
  ticos_register_cli();
#endif
}
