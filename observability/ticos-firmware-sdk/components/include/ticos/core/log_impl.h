#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Logging module APIs intended for use inside the SDK.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Reset the state of log tracking
//!
//! @note Internal function only intended for use with unit tests
void ticos_log_reset(void);

#define TICOS_LOG_NUM_RAM_REGIONS 2

typedef struct {
  const void *region_start;
  uint32_t region_size;
} sTicosLogMemoryRegion;

typedef struct {
  sTicosLogMemoryRegion region[TICOS_LOG_NUM_RAM_REGIONS];
} sTicosLogRegions;

//! @note Internal function used to recover the memory regions which need to be collected in order for logs
//!   to get decoded in a coredump (https://ticos.io/logging)
bool ticos_log_get_regions(sTicosLogRegions *regions);

#ifdef __cplusplus
}
#endif
