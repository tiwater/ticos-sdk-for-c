#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

//! Log utility used within the ticos SDK. When enabled, logs will be emitted to help a user
//! understand what is happening in the library.
//!
//! The Ticos SDK uses logs sparingly to better call out glaring configuration issues and
//! runtime errors. It is recommended to enable all levels of Ticos logs.
//!
//! If your system does not have logging infrastructure in place, the subsystem can also be
//! leveraged for logging within your platform. In that situation, we suggest making your own log.h
//! file for the platform and calling the Ticos macros from there:
//!
//! #include "ticos/core/debug_log.h"
//! #define YOUR_PLATFORM_LOG_DEBUG(...) MEMAULT_LOG_DEBUG(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_INFO(...) MEMAULT_LOG_INFO(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_WARN(...) MEMAULT_LOG_WARN(__VA_ARGS__)
//! #define YOUR_PLATFORM_LOG_ERROR(...) MEMAULT_LOG_ERROR(__VA_ARGS__)

#include "ticos/config.h"
#include "ticos/core/log.h"
#include "ticos/core/platform/debug_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !TICOS_SDK_LOG_SAVE_DISABLE
// Note that this call will be a no-op if the system has not initialized the log module
// by calling ticos_log_boot(). See ./log.h for more details.
#define TICOS_SDK_LOG_SAVE TICOS_LOG_SAVE
#else
#define TICOS_SDK_LOG_SAVE(...)
#endif

#if TICOS_PLATFORM_HAS_LOG_CONFIG
  #include "ticos_platform_log_config.h"
#else

#define _TICOS_LOG_IMPL(_level, ...)             \
  do {                                              \
    TICOS_SDK_LOG_SAVE(_level, __VA_ARGS__);     \
    ticos_platform_log(_level, __VA_ARGS__);     \
  } while (0)

#define TICOS_LOG_DEBUG(...)                                         \
  _TICOS_LOG_IMPL(kTicosPlatformLogLevel_Debug, __VA_ARGS__)

#define TICOS_LOG_INFO(...)                                              \
  _TICOS_LOG_IMPL(kTicosPlatformLogLevel_Info, __VA_ARGS__)

#define TICOS_LOG_WARN(...)                                              \
  _TICOS_LOG_IMPL(kTicosPlatformLogLevel_Warning, __VA_ARGS__)

#define TICOS_LOG_ERROR(...)                                             \
  _TICOS_LOG_IMPL(kTicosPlatformLogLevel_Error, __VA_ARGS__)

//! Only needs to be implemented when using demo component
#define TICOS_LOG_RAW(...)                   \
  ticos_platform_log_raw(__VA_ARGS__)

#endif

#ifdef __cplusplus
}
#endif
