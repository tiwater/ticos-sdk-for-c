//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! @brief
//! An example implementation of the logging Ticos API for the Mbed platform
#include "ticos/core/platform/debug_log.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#ifndef TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

static const char *s_log_prefix = "Tcs:";

static const char *prv_level_to_str(eTicosPlatformLogLevel level) {
  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      return "DEBG";
    case kTicosPlatformLogLevel_Info:
      return "INFO";
    case kTicosPlatformLogLevel_Warning:
      return "WARN";
    case kTicosPlatformLogLevel_Error:
      return "ERRO";
    default:
      return "????";
  }
}

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  const char *level_name = prv_level_to_str(level);

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("%s [%s] %s\n", s_log_prefix, level_name, log_buf);
  fflush(stdout);

  va_end(args);
}

void ticos_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("%s\n", log_buf);
  fflush(stdout);

  va_end(args);
}

void ticos_platform_hexdump(eTicosPlatformLogLevel level, const void *data, size_t data_len) {
  ticos_platform_log(level, "Hexdump Start");

  const char *level_name = prv_level_to_str(level);
  uint8_t *byte_reader = (uint8_t *)&data;

  // wrap each line at (typical screen width - log prefix) / (2 chars + space)
  const size_t max_bytes_per_line = (78 - 15) / 3;
  size_t bytes_on_current_line = 0;

  /* we could build a line and call ticos_platform_log() with it but that would
     require another large buffer so we just printf() each byte here */
  for (size_t i=0; i<data_len; i++) {
    if (bytes_on_current_line == 0) {
      printf("%s [%s] Hexdump: ", s_log_prefix, level_name);
    }

    printf("%02x ", byte_reader[i]);

    if (++bytes_on_current_line == max_bytes_per_line) {
      bytes_on_current_line = 0;
      printf("\n");
      fflush(stdout); // after each line to ensure we don't overrun stdout
    }
  }

  if (bytes_on_current_line > 0) {
    printf("\n");
  }
  ticos_platform_log(level, "Hexdump End");
}
