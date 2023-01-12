#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! ESP32 specific aspects of panic handling

#include <stdint.h>

#include "ticos/core/compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  // A complete dump of all the registers
  kTicosEsp32RegCollectionType_Full = 0,
  // A collection of only the active register window
  kTicosEsp32RegCollectionType_ActiveWindow = 1,
  // ESP8266 (Tensilica LX106 Core) register collection variant
  kTicosEsp32RegCollectionType_Lx106 = 2,
} eTicosEsp32RegCollectionType;

//! Register State collected for ESP32 when a fault occurs
TICOS_PACKED_STRUCT TcsRegState {
  uint32_t collection_type; // eTicosEsp32RegCollectionType
  // NOTE: This matches the layout expected for kTicosEsp32RegCollectionType_ActiveWindow
  uint32_t pc;
  uint32_t ps;
  // NB: The ESP32 has 64 "Address Registers" (ARs) across 4 register windows. Upon exception
  // entry all inactive register windows are force spilled to the stack by software. Therefore, we
  // only need to save the active windows registers at exception entry (referred to as a0-a15).
  uint32_t a[16];
  uint32_t sar;
  uint32_t lbeg;
  uint32_t lend;
  uint32_t lcount;
  uint32_t exccause;
  uint32_t excvaddr;
};


#ifdef __cplusplus
}
#endif
