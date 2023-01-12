#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Platform overrides for the default configuration settings in the ticos-firmware-sdk.
//! Default configuration settings can be found in "ticos/config.h"

#define TICOS_USE_GNU_BUILD_ID 1
#define TICOS_PLATFORM_HAS_LOG_CONFIG 1
#define TICOS_COMPACT_LOG_ENABLE 1

// Enable capture of entire ISR state at time of crash
#define TICOS_NVIC_INTERRUPTS_TO_COLLECT 64
