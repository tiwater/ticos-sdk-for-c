#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! @brief
//!
//! A timer API which needs to be implemented to start collecting ticos heartbeat metrics

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! A timer callback used by the Ticos library
typedef void (TicosPlatformTimerCallback)(void);

//! Start a repeating timer that invokes 'callback' every period_s
//!
//! @param period_sec The interval (in seconds) to invoke the callback at
//! @param callback to invoke
//!
//! @return true if the timer was successfully created and started, false otherwise
bool ticos_platform_metrics_timer_boot(uint32_t period_sec, TicosPlatformTimerCallback *callback);

#ifdef __cplusplus
}
#endif
