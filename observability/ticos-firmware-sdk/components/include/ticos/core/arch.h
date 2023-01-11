#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs for MCU architecture specifics

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Return true if the code is currently running in an interrupt context, false otherwise
bool ticos_arch_is_inside_isr(void);

//! Disable any configurable fault handlers supported by the platform
void ticos_arch_disable_configurable_faults(void);

#ifdef __cplusplus
}
#endif
