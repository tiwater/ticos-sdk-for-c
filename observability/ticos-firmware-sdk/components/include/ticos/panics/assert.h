#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Hooks for linking system assert infra with Ticos error collection

#include "ticos/core/compiler.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/panics/fault_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

//! The 'TICOS_ASSERT*' hooks that should be called as part of the normal ASSERT flow in
//! the system. This will save the reboot info to persist across boots.
//!
//! - TICOS_ASSERT : normal assert
//! - TICOS_ASSERT_EXTRA : assert with a single arbitrary uint32_t context value included
//! - TICOS_ASSERT_EXTRA_AND_REASON : assert with arbitrary uint32_t value and reason code
//! - TICOS_SOFTWARE_WATCHDOG : assert with kTcsRebootReason_SoftwareWatchdog reason set
//!
//! NB: We may also want to think about whether we should save off something like __LINE__ (or use a
//! compiler flag) that blocks the compiler from coalescing asserts (since that could be confusing
//! to an end customer trying to figure out the exact assert hit)

#define TICOS_ASSERT_EXTRA_AND_REASON(_extra, _reason)        \
  do {                                                           \
    void *pc;                                                    \
    TICOS_GET_PC(pc);                                         \
    void *lr;                                                    \
    TICOS_GET_LR(lr);                                         \
    sTicosAssertInfo _assert_info = {                         \
      .extra = (uint32_t)_extra,                                 \
      .assert_reason = _reason,                                  \
    };                                                           \
    ticos_fault_handling_assert_extra(pc, lr, &_assert_info); \
  } while (0)

#define TICOS_ASSERT_RECORD(_extra) \
  TICOS_ASSERT_EXTRA_AND_REASON(_extra, kTcsRebootReason_Assert)

#define TICOS_ASSERT_EXTRA(exp, _extra) \
  do {                                     \
    if (!(exp)) {                          \
      TICOS_ASSERT_RECORD(_extra);      \
    }                                      \
  } while (0)

#define TICOS_ASSERT(exp)                  \
  do {                                        \
    if (!(exp)) {                             \
      void *pc;                               \
      TICOS_GET_PC(pc);                    \
      void *lr;                               \
      TICOS_GET_LR(lr);                    \
      ticos_fault_handling_assert(pc, lr); \
    }                                         \
  } while (0)

//! Assert subclass to be used when a software watchdog trips.
#define TICOS_SOFTWARE_WATCHDOG() \
  TICOS_ASSERT_EXTRA_AND_REASON(0, kTcsRebootReason_SoftwareWatchdog)

#ifdef __cplusplus
}
#endif
