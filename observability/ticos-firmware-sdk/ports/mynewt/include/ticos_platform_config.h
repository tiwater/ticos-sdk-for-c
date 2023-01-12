#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_PLATFORM_HAS_LOG_CONFIG 1

//! Override the default names for configuration files so application developer
//! can easily customize without having to modify the port
#define TICOS_TRACE_REASON_USER_DEFS_FILE  "ticos_trace_reason_mynewt_config.def"
#define TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE "ticos_metrics_heartbeat_mynewt_config.def"

#ifdef __cplusplus
}
#endif
