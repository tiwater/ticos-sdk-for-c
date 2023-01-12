#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Subsystem to trace errors in a way that requires less storage than full coredump traces and
//! also allows the system to continue running after capturing the event.
//!
//! @note For a step-by-step guide about how to integrate and leverage the Trace Event component
//! check out https://ticos.io/error-tracing
//!
//! @note To capture a full snapshot of an error condition (all tasks, logs, and local &
//! global variable state), you can integrate ticos coredumps: https://ticos.io/coredumps

#include <stddef.h>

#include "ticos/config.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/trace_event_impl.h"
#include "ticos/core/trace_reason_user.h"
#include "ticos/core/compact_log_helpers.h"
#include "ticos/core/compact_log_compile_time_checks.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Initializes trace event module
//!
//! @note This must be called before using TICOS_TRACE_EVENT / TICOS_TRACE_EVENT_WITH_STATUS
//!
//! @param storage_impl The event storage implementation being used (returned from
//!   ticos_events_storage_boot())
//! @return 0 on success, else error code.
int ticos_trace_event_boot(const sTicosEventStorageImpl *storage_impl);

//! Records a "Trace Event" with given reason, pc, & lr.
//!
//! The current program counter and return address are collected as part of the macro.
//!
//! @param reason The error reason. See TICOS_TRACE_REASON_DEFINE in trace_reason_user.h for
//! information on how to define reason values.
//! @see ticos_trace_event_capture
//! @see TICOS_TRACE_REASON_DEFINE
//! @note Ensure ticos_trace_event_boot() has been called before using this API!
#define TICOS_TRACE_EVENT(reason)                                    \
  do {                                                                  \
    void *tcs_pc;                                                      \
    TICOS_GET_PC(tcs_pc);                                           \
    void *tcs_lr;                                                      \
    TICOS_GET_LR(tcs_lr);                                           \
    ticos_trace_event_capture(                                       \
        TICOS_TRACE_REASON(reason), tcs_pc, tcs_lr);               \
  } while (0)

//! Records same info as TICOS_TRACE_EVENT as well as a "status_code"
//!
//! @note The status code allows one to disambiguate traces of the same "reason" class
//!  and record additional diagnostic info.
//!
//!  Some example use cases:
//!    reason=UnexpectedBluetoothDisconnect, status_codes = bluetooth error code
//!    reason=LibCFileIoError, status_code = value of errno when operation failed
//!
//! @note Trace events with the same 'reason' but a different 'status_code' are classified as
//!   unique issues in the Ticos UI
#define TICOS_TRACE_EVENT_WITH_STATUS(reason, status_code)           \
  do {                                                                  \
    void *tcs_pc;                                                      \
    TICOS_GET_PC(tcs_pc);                                           \
    void *tcs_lr;                                                      \
    TICOS_GET_LR(tcs_lr);                                           \
    ticos_trace_event_with_status_capture(                           \
        TICOS_TRACE_REASON(reason), tcs_pc, tcs_lr, status_code);  \
  } while (0)

#if !TICOS_COMPACT_LOG_ENABLE

//! Records same info as TICOS_TRACE_EVENT as well as a log
//!
//! @note The log alows one to capture arbitrary metadata when a system error is detected
//! such as the value of several registers or current state machine state.
//!
//! @note Trace events with the same 'reason' but a different 'log' are classified as
//!   grouped together by default in the UI
#define TICOS_TRACE_EVENT_WITH_LOG(reason, ...)                      \
  do {                                                                  \
    void *tcs_pc;                                                      \
    TICOS_GET_PC(tcs_pc);                                           \
    void *tcs_lr;                                                      \
    TICOS_GET_LR(tcs_lr);                                           \
    ticos_trace_event_with_log_capture(                              \
        TICOS_TRACE_REASON(reason), tcs_pc, tcs_lr, __VA_ARGS__);  \
  } while (0)

#else

#define TICOS_TRACE_EVENT_WITH_LOG(reason, format, ...)                           \
  do {                                                                               \
    void *lr;                                                                        \
    TICOS_GET_LR(lr);                                                             \
    TICOS_LOGGING_RUN_COMPILE_TIME_CHECKS(format, ## __VA_ARGS__);                \
    TICOS_LOG_FMT_ELF_SECTION_ENTRY(format, ## __VA_ARGS__);                      \
    ticos_trace_event_with_compact_log_capture(TICOS_TRACE_REASON(reason), lr, \
                                          TICOS_LOG_FMT_ELF_SECTION_ENTRY_PTR,    \
                                          Tcs_GET_COMPRESSED_LOG_FMT(__VA_ARGS__),  \
                                          ## __VA_ARGS__);                           \
   } while (0)

#endif

//! Flushes an ISR trace event capture out to event storage
//!
//! @note If a user is logging events from ISRs, it's recommended this API is called
//! prior to draining data from the packetizer.
//! @note This API is automatically called when a new trace event is recorded.
int ticos_trace_event_try_flush_isr_event(void);

//! Compute the worst case number of bytes required to serialize a Trace Event.
//!
//! @return the worst case amount of space needed to serialize a Trace Event.
size_t ticos_trace_event_compute_worst_case_storage_size(void);

#ifdef __cplusplus
}
#endif
