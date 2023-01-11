#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of ticos_metrics_platform_locking APIs

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @return true if there has been an equivalent number of lock and unlock calls, else false
bool fake_ticos_platform_metrics_lock_calls_balanced(void);

//! Reset the state of the fake locking tracker
void fake_ticos_metrics_platorm_locking_reboot(void);

#ifdef __cplusplus
}
#endif
