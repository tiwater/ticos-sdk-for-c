//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! An example implementation of the logging ticos API for the WICED platform

#include "ticos/core/platform/debug_log.h"
#include "ticos/core/compiler.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "wwd_debug.h"

#ifndef TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES
#  define TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

static const char *TAG TICOS_UNUSED = "tcs";

static void prv_print(const char *fmt, va_list *args) {
#ifdef ENABLE_JLINK_TRACE
  if (WPRINT_PLATFORM_PERMISSION_FUNC()) {
    char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
    vsnprintf(log_buf, sizeof(log_buf), fmt, *args)
    RTT_printf("%s\n", log_buf);
  }
#else
  vprintf(fmt, *args);
  printf("\n");
#endif
}

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  (void)level;

  va_list args;
  va_start(args, fmt);
  prv_print(fmt, &args);
  va_end(args);
}

void ticos_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  prv_print(fmt, &args);
  va_end(args);
}
