#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Assert implementation used internally by the Ticos SDK.
//!
//! Asserts are _only_ used for API misuse and configuration issues (i.e a NULL function pointer
//! as for a function in a *StorageImpl context).

#include "ticos/config.h"
#include "ticos/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TICOS_SDK_ASSERT_ENABLED

//! A user of the SDK may optionally override the default assert macro by adding a CFLAG such as:
//! -DTICOS_SDK_ASSERT=MY_PLATFORM_ASSERT
#ifndef TICOS_SDK_ASSERT

//! An end user can override the implementation to trigger a fault/reboot the system.
//!
//! @note The default implementation is a weak function that is a while (1) {} loop.
TICOS_NORETURN void ticos_sdk_assert_func_noreturn(void);

//! Handler that is invoked when the MEMFULT_SDK_ASSERT() check fails
//!
//! @note The handler:
//!  - logs the return address that tripped the assert
//!  - halts the platform by calling "ticos_platform_halt_if_debugging"
//!  - calls ticos_sdk_assert_func_noreturn
void ticos_sdk_assert_func(void);

#define TICOS_SDK_ASSERT(expr) ((expr) ? (void)0 : ticos_sdk_assert_func())

#endif /* TICOS_SDK_ASSERT */

#else

#define TICOS_SDK_ASSERT(expr) (void)0

#endif /* TICOS_SDK_ASSERT_ENABLED */

#ifdef __cplusplus
}
#endif
