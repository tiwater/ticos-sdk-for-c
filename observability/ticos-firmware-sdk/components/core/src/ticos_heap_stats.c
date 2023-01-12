//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Simple heap allocation tracking utility. Intended to shim into a system's
//! malloc/free implementation to track last allocations with callsite
//! information.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/heap_stats.h"
#include "ticos/core/heap_stats_impl.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/core/platform/overrides.h"

#define TICOS_HEAP_STATS_VERSION 2

TICOS_STATIC_ASSERT(TICOS_HEAP_STATS_MAX_COUNT < TICOS_HEAP_STATS_LIST_END,
                       "Number of entries in heap stats exceeds limits");

sTcsHeapStats g_ticos_heap_stats = {
  .version = TICOS_HEAP_STATS_VERSION,
  .stats_pool_head = TICOS_HEAP_STATS_LIST_END,
};
sTcsHeapStatEntry g_ticos_heap_stats_pool[TICOS_HEAP_STATS_MAX_COUNT];

static void prv_heap_stats_lock(void) {
#if TICOS_COREDUMP_HEAP_STATS_LOCK_ENABLE
  ticos_lock();
#endif
}

static void prv_heap_stats_unlock(void) {
#if TICOS_COREDUMP_HEAP_STATS_LOCK_ENABLE
  ticos_unlock();
#endif
}

void ticos_heap_stats_reset(void) {
  prv_heap_stats_lock();
  g_ticos_heap_stats = (sTcsHeapStats){
    .version = TICOS_HEAP_STATS_VERSION,
    .stats_pool_head = TICOS_HEAP_STATS_LIST_END,
  };
  memset(g_ticos_heap_stats_pool, 0, sizeof(g_ticos_heap_stats_pool));
  prv_heap_stats_unlock();
}

bool ticos_heap_stats_empty(void) {
  // if the first entry has a zero size, we know there was no entry ever
  // populated
  return g_ticos_heap_stats_pool[0].info.size == 0;
}

//! Get the previous entry index of the entry index to be found
//!
//! Iterates through the entry array checking if entry is in use and if the
//! next entry index matches the search index. If the previous entry cannot be found,
//! TICOS_HEAP_STATS_LIST_END is returned
static uint16_t prv_get_previous_entry(uint16_t search_entry_index) {
  uint16_t index = TICOS_HEAP_STATS_LIST_END;

  for (uint16_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    sTcsHeapStatEntry *entry = &g_ticos_heap_stats_pool[i];
    if (entry->info.in_use && entry->info.next_entry_index == search_entry_index) {
      index = i;
      break;
    }
  }

  return index;
}

//! Return the next entry index to write new data to
//!
//! First searches for unused entries
//! If none are found then traverses list to oldest (last) entry
static uint16_t prv_get_new_entry_index(void) {
  sTcsHeapStatEntry *entry;

  for (uint16_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    entry = &g_ticos_heap_stats_pool[i];

    if (!entry->info.in_use) {
      return i;  // favor unused entries
    }
  }

  // No unused entry found, return the oldest (entry with next index of
  // TICOS_HEAP_STATS_LIST_END)
  return prv_get_previous_entry(TICOS_HEAP_STATS_LIST_END);
}

void ticos_heap_stats_malloc(const void *lr, const void *ptr, size_t size) {
  prv_heap_stats_lock();

  if (ptr) {
    g_ticos_heap_stats.in_use_block_count++;
    if (g_ticos_heap_stats.in_use_block_count > g_ticos_heap_stats.max_in_use_block_count) {
      g_ticos_heap_stats.max_in_use_block_count = g_ticos_heap_stats.in_use_block_count;
    }
    uint16_t new_entry_index = prv_get_new_entry_index();
    sTcsHeapStatEntry *new_entry = &g_ticos_heap_stats_pool[new_entry_index];
    *new_entry = (sTcsHeapStatEntry){
      .lr = lr,
      .ptr = ptr,
      .info =
        {
          .size = size & (~(1u << 31)),
          .in_use = 1u,
        },
    };

    // Append new entry to head of the list
    new_entry->info.next_entry_index = g_ticos_heap_stats.stats_pool_head;
    g_ticos_heap_stats.stats_pool_head = new_entry_index;

    // Find next oldest entry (points to new entry)
    // Update next entry index to TICOS_HEAP_STATS_LIST_END
    uint16_t next_oldest_entry_index = prv_get_previous_entry(new_entry_index);

    if (next_oldest_entry_index != TICOS_HEAP_STATS_LIST_END) {
      g_ticos_heap_stats_pool[next_oldest_entry_index].info.next_entry_index =
        TICOS_HEAP_STATS_LIST_END;
    }
  }

  prv_heap_stats_unlock();
}

void ticos_heap_stats_free(const void *ptr) {
  prv_heap_stats_lock();
  if (ptr) {
    g_ticos_heap_stats.in_use_block_count--;

    // if the pointer exists in the tracked stats, mark it as freed
    for (uint16_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
      sTcsHeapStatEntry *entry = &g_ticos_heap_stats_pool[i];
      if ((entry->ptr == ptr) && entry->info.in_use) {
        entry->info.in_use = 0;

        // Find the entry previous to the removed entry
        uint16_t previous_entry_index = prv_get_previous_entry(i);

        // If list head removed, set next entry as new list head
        if (g_ticos_heap_stats.stats_pool_head == i) {
          g_ticos_heap_stats.stats_pool_head = entry->info.next_entry_index;
        }

        // If there's a valid previous entry, set it's next index to removed entry's
        if (previous_entry_index != TICOS_HEAP_STATS_LIST_END) {
          g_ticos_heap_stats_pool[previous_entry_index].info.next_entry_index =
            entry->info.next_entry_index;
        }

        entry->info.next_entry_index = TICOS_HEAP_STATS_LIST_END;
        break;
      }
    }
  }
  prv_heap_stats_unlock();
}
