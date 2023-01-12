#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Configuration settings available in the SDK.
//!
//! All settings can be set from the platform configuration file,
//! "ticos_platform_config.h". If no setting is specified, the default values
//! below will get picked up.
//!
//! The configuration file has three settings which can be overridden by adding a
//! compiler time define to your CFLAG list:
//!  1. TICOS_PLATFORM_CONFIG_FILE can be used to change the default name of
//!     the platform configuration file, "ticos_platform_config.h"
//!  2. TICOS_PLATFORM_CONFIG_DISABLED can be used to disable inclusion of
//!     a platform configuration file.
//!  3. TICOS_PLATFORM_CONFIG_STRICT can be used to force a user of the SDK to
//!     explicitly define every configuration constant rather than pick up defaults.
//!     When the Wundef compiler option is used, any undefined configuration will
//!     be caught at compilation time.

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TICOS_PLATFORM_CONFIG_FILE
#define TICOS_PLATFORM_CONFIG_FILE "ticos_platform_config.h"
#endif

#ifndef TICOS_PLATFORM_CONFIG_DISABLED
#include TICOS_PLATFORM_CONFIG_FILE
#endif

#ifndef TICOS_PLATFORM_CONFIG_STRICT
#define TICOS_PLATFORM_CONFIG_STRICT 0
#endif

#if !TICOS_PLATFORM_CONFIG_STRICT
#define TICOS_PLATFORM_CONFIG_INCLUDE_DEFAULTS
#include "ticos/default_config.h"
#undef TICOS_PLATFORM_CONFIG_INCLUDE_DEFAULTS
#endif

#ifdef __cplusplus
}
#endif
