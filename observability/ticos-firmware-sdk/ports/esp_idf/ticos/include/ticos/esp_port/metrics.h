#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!

#include "ticos/metrics/platform/timer.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Called each time a heartbeat interval expires to invoke the handler
//!
//! The Ticos port implements a default implementation of this in
//! ports/esp_idf/ticos/common/ticos_platform_metrics.c which
//! runs the callback on the esp timer task.
//!
//! Since the duty cycle is low (once / hour) and the work performed is
//! copying a small amount of data in RAM, it's recommended to use the default
//! implementation. However, the function is defined as weak so an end-user
//! can override this behavior by implementing the function in their main application.
//!
//! @param handler The callback to invoke to serialize heartbeat metrics
void ticos_esp_metric_timer_dispatch(TicosPlatformTimerCallback handler);

#ifdef __cplusplus
}
#endif
