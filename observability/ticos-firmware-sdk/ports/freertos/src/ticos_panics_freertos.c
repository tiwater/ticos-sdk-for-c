//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Hooks for collecting coredumps from failure paths in FreeRTOS kernel

#include "ticos/panics/assert.h"

#include "ticos/core/compiler.h"

#include "FreeRTOS.h"
#include "task.h"

//! Collect a coredump when a FreeRTOS assert takes place
//!
//! Implementation for assert function present in FreeRTOSConfig.h:
//!  #define configASSERT(x) if ((x) == 0) vAssertCalled( __FILE__, __LINE__ )
void vAssertCalled(TICOS_UNUSED const char *file,
                   TICOS_UNUSED int line) {
  TICOS_ASSERT(0);
}

//! Collects a coredump when a stack overflow takes place in FreeRTOS kernel
//!
//! @note: Depends on FreeRTOS kernel being compiled with
//!  configCHECK_FOR_STACK_OVERFLOW != 0
void vApplicationStackOverflowHook(TICOS_UNUSED TaskHandle_t xTask,
                                   TICOS_UNUSED char *pcTaskName) {
  TICOS_ASSERT(0);
}
