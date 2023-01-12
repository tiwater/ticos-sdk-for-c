//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Maps ticos platform logging API to zephyr kernel logs

#include "ticos/core/platform/debug_log.h"

//

#include <logging/log.h>
#include <stdio.h>

#include "ticos/config.h"
#include "ticos/ports/zephyr/version.h"
#include "zephyr_release_specific_headers.h"

LOG_MODULE_REGISTER(tcs, CONFIG_TICOS_LOG_LEVEL);

#ifndef TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES
  #define TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES (128)
#endif

//! Translate Ticos logs to Zephyr logs.
void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
  char *log_str = log_buf;

#if !TICOS_ZEPHYR_VERSION_GT(3, 1)
  #if defined(CONFIG_LOG) && !defined(CONFIG_LOG2)
  // Before zephyr 3.1, LOG was a different option from LOG2 and required
  // manually duplicating string argument values. Only required if CONFIG_LOG is
  // in use.
  log_str = log_strdup(log_buf);
  #endif
#endif

// If the user doesn't have logging enabled, optionally route Ticos logs to
// printk, allowing the user to also disable that behavior if no Ticos logs
// are desired.
#if CONFIG_TICOS_PLATFORM_LOG_FALLBACK_TO_PRINTK
  #define MFT_LOG_DBG(fmt_, arg) printk("<dbg> tcs: " fmt_ "\n", arg)
  #define MFT_LOG_INF(fmt_, arg) printk("<inf> tcs: " fmt_ "\n", arg)
  #define MFT_LOG_WRN(fmt_, arg) printk("<wrn> tcs: " fmt_ "\n", arg)
  #define MFT_LOG_ERR(fmt_, arg) printk("<err> tcs: " fmt_ "\n", arg)
#else
  // Either the user has logging enabled, or they don't want to use printk (in
  // which case logs will be dropped via Zephyr log macros).
  #define MFT_LOG_DBG(...) LOG_DBG(__VA_ARGS__)
  #define MFT_LOG_INF(...) LOG_INF(__VA_ARGS__)
  #define MFT_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
  #define MFT_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#endif

  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      MFT_LOG_DBG("%s", log_str);
      break;

    case kTicosPlatformLogLevel_Info:
      MFT_LOG_INF("%s", log_str);
      break;

    case kTicosPlatformLogLevel_Warning:
      MFT_LOG_WRN("%s", log_str);
      break;

    case kTicosPlatformLogLevel_Error:
      MFT_LOG_ERR("%s", log_str);
      break;

    default:
      MFT_LOG_ERR("??? %s", log_str);
      break;
  }

  va_end(args);
}

void ticos_platform_log_raw(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

#define ZEPHYR_VERSION_GTE(major, minor) \
  ((KERNEL_VERSION_MAJOR > (major)) ||   \
   ((KERNEL_VERSION_MAJOR == (major)) && (KERNEL_VERSION_MINOR >= (minor))))

  char log_buf[TICOS_DEBUG_LOG_BUFFER_SIZE_BYTES];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);
#if ZEPHYR_VERSION_GTE(3, 0)
  LOG_PRINTK("%s\n", log_buf);
#else
  printk("%s\n", log_buf);
#endif

  va_end(args);
}
