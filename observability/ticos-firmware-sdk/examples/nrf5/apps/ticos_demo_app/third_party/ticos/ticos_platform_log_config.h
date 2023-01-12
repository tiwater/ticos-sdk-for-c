//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! An example implementation of overriding the Ticos logging macros by
//! placing definitions in ticos_platform_log_config.h and adding
//! -DTICOS_PLATFORM_HAS_LOG_CONFIG=1 to the compiler flags

#pragma once

#include "ticos/core/compiler.h"

#include "sdk_config.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_delay.h"

//! Note: NRF_LOG_FLUSH() needs to be called if NRF_LOG_DEFERRED=1 in order
//! for string formatters to print
#define _TICOS_PORT_LOG_IMPL(_level, tcs_level, fmt, ...)   \
  do {                                              \
    TICOS_SDK_LOG_SAVE(tcs_level, fmt, ## __VA_ARGS__);     \
    NRF_LOG_##_level("Tcs: " fmt, ## __VA_ARGS__); \
    NRF_LOG_FLUSH();                                \
  } while (0)

#define TICOS_LOG_DEBUG(fmt, ...) \
  _TICOS_PORT_LOG_IMPL(DEBUG, kTicosPlatformLogLevel_Debug, fmt, ## __VA_ARGS__)
#define TICOS_LOG_INFO(fmt, ...) \
  _TICOS_PORT_LOG_IMPL(INFO, kTicosPlatformLogLevel_Info, fmt, ## __VA_ARGS__)
#define TICOS_LOG_WARN(fmt, ...) \
  _TICOS_PORT_LOG_IMPL(WARNING, kTicosPlatformLogLevel_Warning, fmt, ## __VA_ARGS__)
#define TICOS_LOG_ERROR(fmt, ...) \
  _TICOS_PORT_LOG_IMPL(ERROR, kTicosPlatformLogLevel_Error, fmt, ## __VA_ARGS__)


//! Note: nrf_delay_ms() is called to give the host a chance to drain the buffers and avoid Segger
//! RTT overruns (data will be dropped on the floor otherwise).  NRF_LOG_FLUSH() is a no-op for the
//! RTT logging backend unfortunately.
#define TICOS_LOG_RAW(fmt, ...)                                      \
  do {                                                                  \
    NRF_LOG_INTERNAL_RAW_INFO(fmt "\n", ## __VA_ARGS__);                \
    NRF_LOG_FLUSH();                                                    \
    nrf_delay_ms(1);                                                    \
  } while (0)
