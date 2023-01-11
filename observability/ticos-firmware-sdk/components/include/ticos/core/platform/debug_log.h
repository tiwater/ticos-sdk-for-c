#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs that need to be implemented in order to enable logging within the ticos SDK
//!
//! The ticos SDK uses logs sparingly to communicate useful diagnostic information to the
//! integrator of the library

#include <stddef.h>

#include "ticos/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kTicosPlatformLogLevel_Debug = 0,
  kTicosPlatformLogLevel_Info,
  kTicosPlatformLogLevel_Warning,
  kTicosPlatformLogLevel_Error,
  // Convenience definition to get the number of possible levels
  kTicosPlatformLogLevel_NumLevels,
} eTicosPlatformLogLevel;

//! Routine for displaying (or capturing) a log.
//!
//! @note it's expected that the implementation will terminate the log with a newline
//! @note Even if there is no UART or RTT Console, it's worth considering adding a logging
//! implementation that writes to RAM or flash which allows for post-mortem analysis
TICOS_PRINTF_LIKE_FUNC(2, 3)
void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...);

//! Routine for printing a log line as-is, only appending a newline, but without suffixing or
//! appending timestamps, log level info, etc. This is used for debug console-commands where
//! it would degrade the developer experience when the string would be suffixed/post-fixed with
//! other characters.
//!
//! @note it's expected that the implementation will terminate the log with a newline
//! but NOT suffix or append anything other than that.
TICOS_PRINTF_LIKE_FUNC(1, 2)
void ticos_platform_log_raw(const char *fmt, ...);

//! Routine for displaying (or capturing) hexdumps
void ticos_platform_hexdump(eTicosPlatformLogLevel level, const void *data, size_t data_len);

#ifdef __cplusplus
}
#endif
