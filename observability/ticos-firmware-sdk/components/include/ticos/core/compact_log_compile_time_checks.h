#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Compile time validity checks run on compact logs:
//!  1) Enables printf() style Wformat checking
//!  2) Verifies that number of args passed is <= the maximum number supported  (15)

#include "ticos/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#if TICOS_COMPACT_LOG_ENABLE

#include "ticos/core/compiler.h"
#include "ticos/core/preprocessor.h"

#define TICOS_LOGGING_MAX_SUPPORTED_ARGS 15


static void run_printf_like_func_check_(const char* format, ...)
    TICOS_PRINTF_LIKE_FUNC(1, 2);

//! Mark the function as used to prevent warnings in situations where this header is included but
//! no logging is actually used
TICOS_USED
static void run_printf_like_func_check_(TICOS_UNUSED const char* format, ...) { }

//! Compilation time checks on log formatter
//!
//! - static asserts that argument list does not exceed allowed length.
//! - Runs printf() style format checking (behind a "if (false)" so that
//!   the actual code gets optimized away)
#define TICOS_LOGGING_RUN_COMPILE_TIME_CHECKS(format, ...)                           \
  do {                                                                                  \
  TICOS_STATIC_ASSERT(                                                               \
      TICOS_ARG_COUNT_UP_TO_32(__VA_ARGS__) <= TICOS_LOGGING_MAX_SUPPORTED_ARGS , \
      TICOS_EXPAND_AND_QUOTE(TICOS_ARG_COUNT_UP_TO_32(__VA_ARGS__))               \
      " args > TICOS_LOGGING_MAX_SUPPORTED_ARGS ("                                   \
      TICOS_EXPAND_AND_QUOTE(TICOS_LOGGING_MAX_SUPPORTED_ARGS) ")!");             \
  if (false) {                                                                          \
    run_printf_like_func_check_(format, ## __VA_ARGS__);                                \
  } \
} while (0)

#endif /* TICOS_COMPACT_LOG_ENABLE */

#ifdef __cplusplus
}
#endif
