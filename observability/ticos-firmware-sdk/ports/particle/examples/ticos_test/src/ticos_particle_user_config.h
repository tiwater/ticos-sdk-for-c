#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Application specific configuration overrides for ticos library

#ifdef __cplusplus
extern "C" {
#endif

//! WARNING: This should only be set for debug purposes. For production fleets, the
//! value must be >= 3600 to avoid being rate limited
#define TICOS_METRICS_HEARTBEAT_INTERVAL_SECS 60

//! This exposes debug commands that can be called for testing Ticos at the cost of using
//! some extra code space. For production builds, it is recommended this flag be set to 0
#define TICOS_PARTICLE_PORT_DEBUG_API_ENABLE 1

//! The software_type name to be displayed in the Ticos UI
#define TICOS_PARTICLE_PORT_SOFTWARE_TYPE "tcs-test-fw"

#ifdef __cplusplus
}
#endif
