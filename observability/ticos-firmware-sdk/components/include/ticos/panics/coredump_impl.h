#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internals used by the "coredump" subsystem in the "panics" component
//! An end user should _never_ call any of these APIs directly.

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/platform/coredump.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Coredump block types
typedef enum TcsCoredumpBlockType  {
  kTcsCoredumpBlockType_CurrentRegisters = 0,
  kTcsCoredumpBlockType_MemoryRegion = 1,
  kTcsCoredumpRegionType_DeviceSerial = 2,
  // Deprecated: kTcsCoredumpRegionType_FirmwareVersion = 3,
  kTcsCoredumpRegionType_HardwareVersion = 4,
  kTcsCoredumpRegionType_TraceReason = 5,
  kTcsCoredumpRegionType_PaddingRegion = 6,
  kTcsCoredumpRegionType_MachineType = 7,
  kTcsCoredumpRegionType_VendorCoredumpEspIdfV2ToV3_1 = 8,
  kTcsCoredumpRegionType_ArmV6orV7Mpu = 9,
  kTcsCoredumpRegionType_SoftwareVersion = 10,
  kTcsCoredumpRegionType_SoftwareType = 11,
  kTcsCoredumpRegionType_BuildId = 12,
} eTcsCoredumpBlockType;

// All elements are in word-sized units for alignment-friendliness.
typedef struct TcsCachedBlock {
  uint32_t valid_cache;
  uint32_t cached_address;

  uint32_t blk_size;
  uint32_t blk[];
} sTcsCachedBlock;

// We'll point to a properly sized memory block of type TcsCachedBlock.
#define TICOS_CACHE_BLOCK_SIZE_WORDS(blk_size) \
  ((sizeof(sTcsCachedBlock) + blk_size) / sizeof(uint32_t))

//! Computes the amount of space that will be required to save a coredump
//!
//! @param save_info The platform specific information to save as part of the coredump
//! @return The space required to save the coredump or 0 on error
size_t ticos_coredump_get_save_size(const sTicosCoredumpSaveInfo *save_info);

//! @param num_regions The number of regions in the list returned
//! @return regions to collect based on the active architecture or NULL if there are no extra
//! regions to collect
const sTcsCoredumpRegion *ticos_coredump_get_arch_regions(size_t *num_regions);

//! Memory regions that are part of the SDK to include in a coredump
//!
//! @param num_regions The number of regions in the list returned
//! @return memory regions with the SDK to collect or NULL if there are no extra
//!   regions to collect
const sTcsCoredumpRegion *ticos_coredump_get_sdk_regions(size_t *num_regions);


#ifdef __cplusplus
}
#endif
