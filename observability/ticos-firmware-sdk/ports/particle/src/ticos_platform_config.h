//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Particle specific ticos platform configuration
//!
//! An end user can further customize by creating a "ticos_particle_user_config.h" in their
//! application directory

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Allow end user to override configuration defaults by creating their own
// ticos_particle_user_config.h in their app directory
#if __has_include("ticos_particle_user_config.h")
#include "ticos_particle_user_config.h"
#endif

// system_version.h is not currently safe to include in headers which get included by C files
// because it uses member initializers that are only CPP compatible. When the block below is
// wrapped accordingly we will be able to include system_version.h and access the SYSTEM_VERSION
// define:
//   https://github.com/particle-iot/device-os/blob/develop/system/inc/system_version.h#L341-L350

#ifndef TICOS_PARTICLE_PORT_CPP_ONLY_SYSTEM_VERSION
#define TICOS_PARTICLE_PORT_CPP_ONLY_SYSTEM_VERSION 1
#endif

#if !TICOS_PARTICLE_PORT_CPP_ONLY_SYSTEM_VERSION
#include "system_version.h"
#define TICOS_PARTICLE_SYSTEM_VERSION SYSTEM_VERSION
#else

// Assume target is Device OS 3.2
#define TICOS_PARTICLE_SYSTEM_VERSION 0x03020000
#endif

#ifndef TICOS_PARTICLE_SYSTEM_VERSION_MAJOR
#define TICOS_PARTICLE_SYSTEM_VERSION_MAJOR (((TICOS_PARTICLE_SYSTEM_VERSION) >> 24) & 0xff)
#endif

#ifndef TICOS_PARTICLE_SYSTEM_VERSION_MINOR
#define TICOS_PARTICLE_SYSTEM_VERSION_MINOR (((TICOS_PARTICLE_SYSTEM_VERSION) >> 16) & 0xff)
#endif

// Override default names for configuration files so the default files can optionally be defined
// from an end user application to create custom events
#define TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE "ticos_particle_metrics_heartbeat_config.def"
#define TICOS_TRACE_REASON_USER_DEFS_FILE "ticos_particle_trace_reason_user_config.def"

//! Returns true if current target Device OS ("System Version") is greater than the one specified
//!
//! Two checks:
//!  - First check if major version is greater than the one specified
//!  - Finally check if the major version matches and the minor version is greater
#define TICOS_PARTICLE_SYSTEM_VERSION_GT(major, minor)               \
  ((TICOS_PARTICLE_SYSTEM_VERSION_MAJOR > (major)) ||                \
   ((TICOS_PARTICLE_SYSTEM_VERSION_MAJOR == (major)) &&              \
    (TICOS_PARTICLE_SYSTEM_VERSION_MINOR > (minor))))

#if TICOS_PARTICLE_SYSTEM_VERSION_GT(3, 2)
// Support for emitting a build ID shipped in Device OS 3.3
//  https://github.com/particle-iot/device-os/pull/2391
#define TICOS_USE_GNU_BUILD_ID 1

// Build Id info is emitted as part of the software_version so don't double encode
#define TICOS_EVENT_INCLUDE_BUILD_ID 0
#define TICOS_COREDUMP_INCLUDE_BUILD_ID 0
#endif

// Mapping to Ticos logging API can be found in ticos_platform_log_config.h
#define TICOS_PLATFORM_HAS_LOG_CONFIG 1

#ifndef TICOS_PARTICLE_PORT_LOGGING_ENABLE
#define TICOS_PARTICLE_PORT_LOGGING_ENABLE 1
#endif

// Allow events to be batched in a single message rather than sending one at a time
#ifndef TICOS_EVENT_STORAGE_READ_BATCHING_ENABLED
#define TICOS_EVENT_STORAGE_READ_BATCHING_ENABLED 1
#endif

#ifndef TICOS_EVENT_STORAGE_READ_BATCHING_MAX_BYTES
// To send over particle transport our message is going to be base64 encoded
// Gen 3 products support a 1024 (MAX_EVENT_DATA_LENGTH) data buffer.
// We pick a size that accounts for chunk message overhead (8 bytes) and can be encoded in a single MTU
#define TICOS_EVENT_STORAGE_READ_BATCHING_MAX_BYTES (768 - 8)
#endif

//! Enables support for the non-volatile event storage at compile time
//! instead of dynamically at runtime
//!
//! Disabling this feature saves several hundred bytes of codespace and can be useful to enable for
//! extremely constrained environments
#define TICOS_EVENT_STORAGE_NV_SUPPORT_ENABLED 1

// We namespace Ticos fault handlers so they do not conflict with any handlers users may have
// implemented using the default CMSIS names
#define TICOS_EXC_HANDLER_HARD_FAULT Ticos_HardFault_Handler
#define TICOS_EXC_HANDLER_MEMORY_MANAGEMENT Ticos_MemoryManagement_Handler
#define TICOS_EXC_HANDLER_BUS_FAULT Ticos_BusFault_Handler
#define TICOS_EXC_HANDLER_USAGE_FAULT Ticos_UsageFault_Handler
#define TICOS_EXC_HANDLER_NMI Ticos_NMI_Handler
#define TICOS_EXC_HANDLER_WATCHDOG Ticos_Watchdog_Handler

#ifndef TICOS_PARTICLE_PORT_FAULT_HANDLERS_ENABLE
#define TICOS_PARTICLE_PORT_FAULT_HANDLERS_ENABLE 1
#endif

#ifndef TICOS_PARTICLE_PORT_PANIC_HANDLER_HOOK_ENABLE

#if TICOS_PARTICLE_SYSTEM_VERSION_GT(3, 2)
// Support for hooking into the panic subsystem shipped in Device OS 3.3
//  https://github.com/particle-iot/device-os/pull/2384
#define TICOS_PARTICLE_PORT_PANIC_HANDLER_HOOK_ENABLE 1
#else
#define TICOS_PARTICLE_PORT_PANIC_HANDLER_HOOK_ENABLE 0
#endif

#endif

#ifndef TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE

#if TICOS_PARTICLE_SYSTEM_VERSION_GT(3, 2)
// Support for exposing RTOS TCBs shipped in Device OS 3.3
//  https://github.com/particle-iot/device-os/pull/2394
#define TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE 1
#else
#define TICOS_PARTICLE_PORT_COREDUMP_TASK_COLLECTION_ENABLE 0
#endif

#endif

#ifndef TICOS_PARTICLE_PORT_EVENT_STORAGE_SIZE
// Storage for events yet to be sent up (i.e heartbeats, reboot, & trace)
// Events vary in size but typically average ~50 bytes. By default, use
// a size that allows for ~1/2 day of data to be batched up
#define TICOS_PARTICLE_PORT_EVENT_STORAGE_SIZE 512
#endif

#ifndef TICOS_PARTICLE_PORT_LOG_STORAGE_ENABLE
#define TICOS_PARTICLE_PORT_LOG_STORAGE_ENABLE 0
#endif

#if TICOS_PARTICLE_PORT_LOG_STORAGE_ENABLE
#define TICOS_COREDUMP_COLLECT_LOG_REGIONS 1
#endif

#ifndef TICOS_PARTICLE_PORT_LOG_STORAGE_SIZE
#define TICOS_PARTICLE_PORT_LOG_STORAGE_SIZE 1024
#endif

#ifndef TICOS_PARTICLE_PORT_DEBUG_API_ENABLE
#define TICOS_PARTICLE_PORT_DEBUG_API_ENABLE 0
#endif

#ifndef TICOS_PARTICLE_PORT_HEAP_METRICS_ENABLE
#define TICOS_PARTICLE_PORT_HEAP_METRICS_ENABLE 1
#endif

#ifndef TICOS_PARTICLE_PORT_CLOUD_METRICS_ENABLE
#define TICOS_PARTICLE_PORT_CLOUD_METRICS_ENABLE 1
#endif

#ifndef TICOS_PARTICLE_PORT_SOFTWARE_TYPE
#define TICOS_PARTICLE_PORT_SOFTWARE_TYPE "app-fw"
#endif

//
// Coredump collection configuration
// By default, the particle port makes use of the RAM backed coredump port
//  ports/panics/src/ticos_platform_ram_backed_coredump.c
//

// The memory blocks to collect in a coredump come from the particle port
#define TICOS_PLATFORM_COREDUMP_STORAGE_REGIONS_CUSTOM 1

// Noinit section data gets placed in from a user application when using "retained"
#define TICOS_PLATFORM_COREDUMP_NOINIT_SECTION_NAME ".retained_user"

#ifndef TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE
#define TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE 1024
#endif

#ifdef __cplusplus
}
#endif
