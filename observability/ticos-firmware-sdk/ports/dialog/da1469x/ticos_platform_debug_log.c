//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Map ticos logging dependencies to da145xx arch_printf implementation.

#include "ticos/core/platform/debug_log.h"
#include "ticos/config.h"

#include <stdarg.h>
#include <stdio.h>

#ifndef TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES
  #define TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
#if defined (CONFIG_RETARGET) || defined (CONFIG_RTT)
  va_list args;
  va_start(args, fmt);

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  const char *lvl_str = NULL;
  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      lvl_str = "D";
      break;

    case kTicosPlatformLogLevel_Info:
      lvl_str = "I";
      break;

    case kTicosPlatformLogLevel_Warning:
      lvl_str = "W";
      break;

    case kTicosPlatformLogLevel_Error:
      lvl_str = "E";
      break;

    default:
      break;
  }

  if (lvl_str) {
      printf("[%s] Tcs: %s\r\n", lvl_str, log_buf);
  }
  va_end(args);
#endif
}

void ticos_platform_log_raw(const char *fmt, ...) {
#if defined (CONFIG_RETARGET) || defined (CONFIG_RTT)
  va_list args;
  va_start(args, fmt);

  vprintf(fmt, args);
  printf("\n");

  va_end(args);
#endif
}
