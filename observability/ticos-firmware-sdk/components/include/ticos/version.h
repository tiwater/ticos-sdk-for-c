#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Contains Ticos SDK version information.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct {
  uint8_t major;
  uint8_t minor;
  uint8_t patch;
} sTcsSdkVersion;

#define TICOS_SDK_VERSION   { .major = 0, .minor = 37, .patch = 0 }

#ifdef __cplusplus
}
#endif
