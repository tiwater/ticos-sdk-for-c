#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! ESP32 Ticos Demo Cli Port

#ifdef __cplusplus
extern "C" {
#endif

//! Call on boot to enabled the tcs demo cli
//! See https://ticos.io/demo-cli for more info
void ticos_register_cli(void);

#ifdef __cplusplus
}
#endif
