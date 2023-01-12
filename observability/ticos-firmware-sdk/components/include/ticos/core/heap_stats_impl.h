#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Heap tracking APIs intended for use within the ticos-firmware-sdk

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Number of regions used by heap stats
#define TICOS_HEAP_STATS_NUM_RAM_REGIONS 2

//! Value used to indicate entry is the end of the list
#define TICOS_HEAP_STATS_LIST_END UINT16_MAX

typedef struct TcsHeapStatEntry sTcsHeapStatEntry;

//! The heap stats data structure, which is exported when capturing a core.
typedef struct TcsHeapStatEntry {
  // LR at time of allocation
  const void *lr;

  // The pointer returned by the actual allocation function
  const void *ptr;

  // Size of this allocation. 0 means the entry is invalid and should be ignored
  struct {
    // 31 bits to represent the size passed to malloc
    uint32_t size : 31;
    // 1 bit indicating whether this entry is still in use or has been freed
    uint32_t in_use : 1;
    // Index to next oldest entry in heap stats entry array,
    uint16_t next_entry_index;
  } info;
} sTcsHeapStatEntry;

typedef struct TcsHeapStats {
  uint8_t version;

  // Number of blocks currently allocated and not freed
  uint32_t in_use_block_count;

  // Track the max value of 'in_use_block_count'
  uint32_t max_in_use_block_count;

  // Allocation entry list pointer
  uint16_t stats_pool_head;
} sTcsHeapStats;

extern sTcsHeapStats g_ticos_heap_stats;
extern sTcsHeapStatEntry g_ticos_heap_stats_pool[TICOS_HEAP_STATS_MAX_COUNT];

//! Reset the tracked stats.
void ticos_heap_stats_reset(void);

//! Check if no heap stats have been collected
bool ticos_heap_stats_empty(void);

#ifdef __cplusplus
}
#endif
