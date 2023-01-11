//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
// Logging depends on how your configuration does logging. See
// https://docs.ticos.com/docs/mcu/self-serve/#logging-dependency

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "ticos/core/log.h"

#define TICOS_LOG_DEBUG(fmt, ...)                                        \
  do {                                                                      \
    TICOS_LOG_SAVE(kTicosPlatformLogLevel_Debug, fmt, ##__VA_ARGS__); \
    printf("[D] " fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define TICOS_LOG_INFO(fmt, ...)                                        \
  do {                                                                     \
    TICOS_LOG_SAVE(kTicosPlatformLogLevel_Info, fmt, ##__VA_ARGS__); \
    printf("[I] " fmt "\n", ##__VA_ARGS__);                                \
  } while (0)

#define TICOS_LOG_WARN(fmt, ...)                                           \
  do {                                                                        \
    TICOS_LOG_SAVE(kTicosPlatformLogLevel_Warning, fmt, ##__VA_ARGS__); \
    printf("[W] " fmt "\n", ##__VA_ARGS__);                                   \
  } while (0)

#define TICOS_LOG_ERROR(fmt, ...)                                        \
  do {                                                                      \
    TICOS_LOG_SAVE(kTicosPlatformLogLevel_Error, fmt, ##__VA_ARGS__); \
    printf("[E] " fmt "\n", ##__VA_ARGS__);                                 \
  } while (0)

#define TICOS_LOG_RAW(fmt, ...)   \
  do {                               \
    printf(fmt "\n", ##__VA_ARGS__); \
  } while (0)

#ifdef __cplusplus
}
#endif
