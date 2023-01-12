#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Zephyr port overrides for the default configuration settings in the ticos-firmware-sdk.
#include <autoconf.h> // For Kconfig settings
#include <version.h>  // Zephyr version macros

#ifdef __cplusplus
extern "C" {
#endif

// Note that pre-v2.0 Zephyr did not create the section allocation needed to support
// our Gnu build ID usage.
#if KERNEL_VERSION_MAJOR >= 2
// Add a unique identifier to the firmware build
//
// It is very common, especially during development, to not change the firmware
// version between editing and compiling the code. This will lead to issues when
// recovering backtraces or symbol information because the debug information in
// the symbol file may be out of sync with the actual binary. Tracking a build id
// enables the Ticos cloud to identify and surface when this happens! Below
// requires the "-Wl,--build-id" flag.
#define TICOS_USE_GNU_BUILD_ID 1
#endif

// We need to define TICOS_COREDUMP_COLLECT_LOG_REGIONS=1 for the logs to
// show up in the Ticos UI on crash.
#ifndef TICOS_COREDUMP_COLLECT_LOG_REGIONS
#define TICOS_COREDUMP_COLLECT_LOG_REGIONS 1
#endif

#define TICOS_WATCHDOG_SW_TIMEOUT_SECS CONFIG_TICOS_SOFTWARE_WATCHDOG_TIMEOUT_SECS

// Logs are saved to the Ticos logging system as part of
// ticos logging integration (CONFIG_TICOS_LOGGING_ENABLE=y)
// so no need to save from the SDK
#define TICOS_SDK_LOG_SAVE_DISABLE 1

#if CONFIG_TICOS_CACHE_FAULT_REGS
// Map Zephyr config to Ticos define so that we can
// collect the HW fault regs before Zephyr modifies them.
#define TICOS_CACHE_FAULT_REGS 1
#endif

#if CONFIG_TICOS_HEAP_STATS
// Map Zephyr config to Ticos define to enable heap
// tracing collection.
#define TICOS_COREDUMP_COLLECT_HEAP_STATS 1
#endif

#if CONFIG_TICOS_NRF_CONNECT_SDK

#define TICOS_HTTP_CHUNKS_API_HOST "chunks-nrf.ticos.com"
#define TICOS_HTTP_DEVICE_API_HOST "device-nrf.ticos.com"

#endif

#if CONFIG_TICOS_USER_CONFIG_ENABLE

// Pick up any user configuration overrides
#if CONFIG_TICOS_USER_CONFIG_SILENT_FAIL

# if __has_include("ticos_platform_config.h")
#   include "ticos_platform_config.h"
# endif

#else

#include "ticos_platform_config.h"

#endif /* CONFIG_TICOS_USER_CONFIG_SILENT_FAIL */

#else
#define TICOS_DISABLE_USER_TRACE_REASONS 1
#endif

#ifdef __cplusplus
}
#endif
