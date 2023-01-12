//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! An example implementation of the logging ticos API for the ESP32 platform

#include "ticos/core/platform/debug_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ticos/config.h"

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

void ticos_platform_hexdump(eTicosPlatformLogLevel level, const void *data, size_t data_len) {
  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, data_len, ESP_LOG_DEBUG);
      break;

    case kTicosPlatformLogLevel_Info:
      ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, data_len, ESP_LOG_INFO);
      break;

    case kTicosPlatformLogLevel_Warning:
      ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, data_len, ESP_LOG_WARN);
      break;

    case kTicosPlatformLogLevel_Error:
      ESP_LOG_BUFFER_HEX_LEVEL(TAG, data, data_len, ESP_LOG_ERROR);
      break;
    default:
      break;
  }
}
