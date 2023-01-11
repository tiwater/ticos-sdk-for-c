#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! psoc6 specific port APIs

#ifdef __cplusplus
extern "C" {
#endif

//! Tracks Wi-Fi Connection Manager (wcm) subsytem with ticos heartbeats
//!
//! @note: This must be called after cy_wcm_init() is called
void ticos_wcm_metrics_boot(void);

#ifdef __cplusplus
}
#endif
