#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Default PSOC6 configuration

#ifdef __cplusplus
extern "C" {
#endif

//! Pick up logging macros from ticos_platform_log_config.h
#define TICOS_PLATFORM_HAS_LOG_CONFIG 1

//! Change the name of the coredump noinit section to align with default
//! PSOC6 .ld of KEEP(*(.noinit))
#define TICOS_PLATFORM_COREDUMP_NOINIT_SECTION_NAME ".tcs_coredump.noinit"

#define TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 8192

//! Defines specific to PSOC6 Port
//! Note that we include them last so an end user settings above override the defaults.
#include "psoc6_default_config.h"

#ifdef __cplusplus
}
#endif
