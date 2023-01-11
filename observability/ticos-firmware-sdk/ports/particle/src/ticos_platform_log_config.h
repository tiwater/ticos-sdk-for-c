#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Port of the Ticos logging APIs to the particle subsystem

#ifdef __cplusplus
extern "C" {
#endif

#include "logging.h"
#include "ticos-firmware-sdk/components/include/ticos/config.h"

#if TICOS_PARTICLE_PORT_LOGGING_ENABLE

#define TICOS_LOG_DEBUG(...) LOG_DEBUG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define TICOS_LOG_INFO(...) LOG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define TICOS_LOG_WARN(...) LOG_C(WARN, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define TICOS_LOG_ERROR(...) LOG_C(ERROR, LOG_THIS_CATEGORY(), __VA_ARGS__)
#define TICOS_LOG_RAW(...) LOG_DEBUG_C(INFO, LOG_THIS_CATEGORY(), __VA_ARGS__)

#else

#define TICOS_LOG_DEBUG(...)
#define TICOS_LOG_INFO(...)
#define TICOS_LOG_WARN(...)
#define TICOS_LOG_ERROR(...)
#define TICOS_LOG_RAW(...)

#endif

#ifdef __cplusplus
}
#endif
