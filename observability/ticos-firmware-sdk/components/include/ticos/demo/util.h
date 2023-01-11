#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Utilities used within the "demo" component.

#ifdef __cplusplus
extern "C" {
#endif

//! Returns the URL to the chunks endpoint
//!
//! @note Default weak implementation exists in demo/src so the "print_chunk"
//!   cli command can be used without the need of the "http" component.
//! @note A non-weak implementation is exposed in demo/src/http/ticos_demo_https.c and picked up
//!   automatically by the build system helpers when the "http" component is enabled.
const char *ticos_demo_get_chunks_url(void);

//! Returns the current Project Key
//!
//! @note Default weak implementation exists in demo/src so the "print_chunk"
//!   cli command can be used without the need of the "http" component.
//! @note A non-weak implementation is exposed in demo/src/http/ticos_demo_https.c and picked up
//!   automatically by the build system helpers when the "http" component is enabled.
const char *ticos_demo_get_api_project_key(void);

#ifdef __cplusplus
}
#endif
