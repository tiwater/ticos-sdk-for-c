#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs used internally by the SDK for trace event collection. An user
//! of the SDK should never have to call these routines directly.

#include "ticos/core/trace_reason_user.h"
#include "ticos/core/compiler.h"

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

int ticos_trace_event_capture(eTcsTraceReasonUser reason, void *pc_addr, void *lr_addr);
int ticos_trace_event_with_status_capture(
    eTcsTraceReasonUser reason, void *pc_addr, void *lr_addr, int32_t status);

TICOS_PRINTF_LIKE_FUNC(4, 5)
int ticos_trace_event_with_log_capture(
    eTcsTraceReasonUser reason, void *pc_addr, void *lr_addr, const char *fmt, ...);

int ticos_trace_event_with_compact_log_capture(
    eTcsTraceReasonUser reason, void *lr_addr,
    uint32_t log_id, uint32_t fmt, ...);

#ifdef __cplusplus
}
#endif
