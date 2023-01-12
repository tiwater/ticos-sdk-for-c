//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A fake implementation simulating platform logs which can be used for unit tests

#include "ticos/core/platform/debug_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ticos/core/log.h"
#include "ticos/core/compiler.h"

static const char *prv_severity_level_to_str(eTicosPlatformLogLevel level) {
  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      return "D";
    case kTicosPlatformLogLevel_Info:
      return "I";
    case kTicosPlatformLogLevel_Warning:
      return "W";
    case kTicosPlatformLogLevel_Error:
      return "E";
    case kTicosPlatformLogLevel_NumLevels: // silence error with -Wswitch-enum
    default:
      return "U";
  }
}

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  strcpy(log_buf, "Tcs: ");
  char *write_ptr = &log_buf[0] + strlen(log_buf);
  vsnprintf(write_ptr, sizeof(log_buf) - strlen(log_buf), fmt, args);

  printf("[%s] %s\n", prv_severity_level_to_str(level), log_buf);
}

void ticos_platform_hexdump(TICOS_UNUSED eTicosPlatformLogLevel level,
                               TICOS_UNUSED const void *data,
                               TICOS_UNUSED size_t data_len) {
  // No fake impl yet!
}
