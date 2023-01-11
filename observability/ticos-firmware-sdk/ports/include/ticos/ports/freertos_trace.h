#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! This file needs to be included from your platforms FreeRTOSConfig.h to take advantage of
//! Ticos's hooks into the FreeRTOS tracing utilities

#ifdef __cplusplus
extern "C" {
#endif

#include "ticos/config.h"

// FreeRTOSConfig.h is often included in assembly files so wrap function declarations for
// convenience to prevent compilation errors
#if !defined(__ASSEMBLER__) && !defined(__IAR_SYSTEMS_ASM__)

void ticos_freertos_trace_task_create(void *tcb);
void ticos_freertos_trace_task_delete(void *tcb);

#include  "ticos/core/heap_stats.h"

#endif

//
// We ifndef the trace macros so it's possible for an end user to use a custom definition that
// calls Ticos's implementation as well as their own
//

#ifndef traceTASK_CREATE
#define traceTASK_CREATE(pxNewTcb) ticos_freertos_trace_task_create(pxNewTcb)
#endif

#ifndef traceTASK_DELETE
#define traceTASK_DELETE(pxTaskToDelete) ticos_freertos_trace_task_delete(pxTaskToDelete)
#endif

#if TICOS_FREERTOS_PORT_HEAP_STATS_ENABLE

#if TICOS_COREDUMP_HEAP_STATS_LOCK_ENABLE != 0
// FreeRTOS has its own locking mechanism (suspends tasks) so don't attempt
// to use the ticos_lock implementation as well
#error "TICOS_COREDUMP_HEAP_STATS_LOCK_ENABLE must be 0 when using TICOS_FREERTOS_PORT_HEAP_STATS_ENABLE"
#endif

#ifndef traceFREE
#define traceFREE(pv, xBlockSize) TICOS_HEAP_STATS_FREE(pv)
#endif

#ifndef traceMALLOC
#define traceMALLOC(pvReturn, xWantedSize) TICOS_HEAP_STATS_MALLOC(pvReturn, xWantedSize)
#endif

#endif /* TICOS_FREERTOS_PORT_HEAP_STATS_ENABLE */

//! A define that is used to assert that this file has been included from FreeRTOSConfig.h
#define TICOS_FREERTOS_TRACE_ENABLED 1

#ifdef __cplusplus
}
#endif
