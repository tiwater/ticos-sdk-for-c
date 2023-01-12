#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! APIs the platform must implement for using the SDK. These routines are needed by all
//! subcomponents of the library one can include

#include <stddef.h>
#include <stdint.h>

#include "ticos/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Initialize Ticos SDK modules used in the port
//!
//! @note The expectation is a user of the SDK will implement this function and call it once on
//! boot. It should be used to setup anything needed by the SDK (such as event & coredump
//! storage) and then start up any of the ticos subsystems being used
//! @note This is also a good time to run any runtime assertion checks (such as checking
//! that coredump storage is large enough to hold the coredump regions being collected)
//! @note A reference usage can be found in nrf5/libraries/ticos/ticos_platform_core.c
//!
//! @return 0 if initialization completed, else error code
int ticos_platform_boot(void);

//! Invoked after ticos fault handling has run.
//!
//! The platform should do any final cleanup and reboot the system
TICOS_NORETURN void ticos_platform_reboot(void);

//! Invoked after faults occur so the user can debug locally if a debugger is attached
void ticos_platform_halt_if_debugging(void);

//! @return elapsed time since the device was last restarted (in milliseconds)
uint64_t ticos_platform_get_time_since_boot_ms(void);

#ifdef __cplusplus
}
#endif
