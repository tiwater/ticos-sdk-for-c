#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Platform overrides for the default configuration settings in the ticos-firmware-sdk.
//! Default configuration settings can be found in "ticos/config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_PLATFORM_COREDUMP_STORAGE_USE_FLASH    1

// Note: The default location coredumps are saved is NVMS log storage.
// This size can be adjusted depending on the amount of RAM regions collected
// in ticos_platform_coredump_get_regions()
#define TICOS_PLATFORM_COREDUMP_STORAGE_MAX_SIZE_BYTES (32 * 1024)

#define TICOS_USE_GNU_BUILD_ID 1

#ifdef __cplusplus
}
#endif
