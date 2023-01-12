#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! aarch64 specific aspects of panic handling.

#include <stdint.h>

#include "ticos/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Note: With aarch64, it is the programmer's responsiblity to store an exception frame
//! so there is no standard layout.
TICOS_PACKED_STRUCT TcsRegState {
  uint64_t sp; // sp prior to exception entry
  uint64_t pc; // pc prior to exception entry
  // Note: The CPSR is a 32 bit register but storing as a 64 bit register to keep packing simple
  uint64_t cpsr;
  uint64_t x[31]; // x0 - x30
};

#ifdef __cplusplus
}
#endif
