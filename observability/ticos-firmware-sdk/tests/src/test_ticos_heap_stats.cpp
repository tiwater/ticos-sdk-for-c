//! @file
//!
//! @brief

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_ticos_platform_metrics_locking.h"

#include "ticos/core/heap_stats.h"
#include "ticos/core/heap_stats_impl.h"
#include "ticos/core/math.h"

TEST_GROUP(TicosHeapStats) {
  void setup() {
    fake_ticos_metrics_platorm_locking_reboot();

    mock().disable();
  }
  void teardown() {
    CHECK(fake_ticos_platform_metrics_lock_calls_balanced());
    ticos_heap_stats_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static bool prv_heap_stat_equality(const sTcsHeapStatEntry *expected,
                                   const sTcsHeapStatEntry *actual) {
  bool match = (expected->lr == actual->lr) &&    //
               (expected->ptr == actual->ptr) &&  //
               (expected->info.size == actual->info.size) && //
               (expected->info.in_use == actual->info.in_use);
  if (!match) {
    fprintf(stderr,
            "sTcsHeapStatEntry:\n"
            " lr: %p : %p\n"
            " ptr: %p : %p\n"
            " size: %u : %u\n"
            " in_use: %u : %u\n",
            expected->lr, actual->lr, expected->ptr, actual->ptr, (unsigned int)expected->info.size,
            (unsigned int)actual->info.size, (unsigned int)expected->info.in_use,
            (unsigned int)actual->info.in_use);
  }
  return match;
}

TEST(TicosHeapStats, Test_Basic) {
  void *lr;
  TICOS_GET_LR(lr);
  const sTcsHeapStatEntry expected_heap_stats[] = {
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info =
        {
          .size = 1234,
          .in_use = 1,
        },
    },
  };

  bool empty = ticos_heap_stats_empty();
  CHECK(empty);

  // Check initial list head points to empty
  LONGS_EQUAL(TICOS_HEAP_STATS_LIST_END, g_ticos_heap_stats.stats_pool_head);

  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  empty = ticos_heap_stats_empty();
  CHECK(!empty);

  bool match = prv_heap_stat_equality(&expected_heap_stats[0], &g_ticos_heap_stats_pool[0]);

  CHECK(match);
}

TEST(TicosHeapStats, Test_Free) {
  void *lr;
  TICOS_GET_LR(lr);
  const sTcsHeapStatEntry
    expected_heap_stats[TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool)] = {
      {
        .lr = lr,
        .ptr = (void *)0x12345679,
        .info =
          {
            .size = 1234,
            .in_use = 1,
          },
      },
      {
        .lr = lr,
        .ptr = (void *)0x1234567A,
        .info =
          {
            .size = 12345,
            .in_use = 0,
          },
      },
    };

  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  LONGS_EQUAL(1, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(1, g_ticos_heap_stats.max_in_use_block_count);

  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  LONGS_EQUAL(2, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_ticos_heap_stats.max_in_use_block_count);

  TICOS_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  LONGS_EQUAL(1, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_ticos_heap_stats.max_in_use_block_count);

  // test freeing a null pointer
  TICOS_HEAP_STATS_FREE(NULL);
  LONGS_EQUAL(1, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_ticos_heap_stats.max_in_use_block_count);

  // test freeing something that doesn't exist in the tracked list
  TICOS_HEAP_STATS_FREE((void *)0xabc);
  // since we don't know that the untracked but non-null pointer being freed
  // above is invalid, we still decrement the in use count
  LONGS_EQUAL(0, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_ticos_heap_stats.max_in_use_block_count);

  // test malloc stats with failed malloc (NULL pointer)
  TICOS_HEAP_STATS_MALLOC(NULL, 123456);
  LONGS_EQUAL(0, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_ticos_heap_stats.max_in_use_block_count);

  // work over the list, confirming that everything matches expected
  size_t list_count = 0;
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    sTcsHeapStatEntry *pthis = &g_ticos_heap_stats_pool[i];
    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected_heap_stats[i], pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(2, list_count);

  // Test correct state after freeing last entry
  TICOS_HEAP_STATS_FREE(expected_heap_stats[0].ptr);
  LONGS_EQUAL(TICOS_HEAP_STATS_LIST_END, g_ticos_heap_stats.stats_pool_head);
}

TEST(TicosHeapStats, Test_MaxEntriesRollover) {
  void *lr;
  TICOS_GET_LR(lr);
  const sTcsHeapStatEntry
    expected_heap_stats[TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool)] = {
      {
        .lr = lr,
        .ptr = (void *)0x12345679,
        .info =
          {
            .size = 1234,
            .in_use = 1,
          },
      },
      {
        .lr = lr,
        // this entry should not appear when checking at the end of this test,
        // so set the data to something exceptional
        .ptr = (void *)0xabcdef,
        .info =
          {
            .size = 123456,
            .in_use = 0,
          },
      },
    };

  // allocate one entry, and then free it
  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  TICOS_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  LONGS_EQUAL(0, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(1, g_ticos_heap_stats.max_in_use_block_count);

  // now allocate enough entries to fill the stats completely, overwriting the
  // existing entry
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    TICOS_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats[0].ptr + i),
                               expected_heap_stats[0].info.size + i);
  }
  LONGS_EQUAL(TICOS_HEAP_STATS_MAX_COUNT, g_ticos_heap_stats.in_use_block_count);
  LONGS_EQUAL(TICOS_HEAP_STATS_MAX_COUNT, g_ticos_heap_stats.max_in_use_block_count);

  // work over the list, FIFO, confirming that everything matches expected
  size_t list_count = 0;
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    sTcsHeapStatEntry *pthis = &g_ticos_heap_stats_pool[i];
    sTcsHeapStatEntry expected = {
      .lr = lr,
      .ptr = (void *)(0x12345679 + i),
      .info =
        {
          .size = 1234 + (uint32_t)i,
          .in_use = 1,
        },
    };

    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected, pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(TICOS_HEAP_STATS_MAX_COUNT, list_count);
}

//! Verify that an allocation that reuses a previously freed address is properly
//! cleared from the stats list
TEST(TicosHeapStats, Test_AddressReuse) {
  void *lr;
  TICOS_GET_LR(lr);
  const sTcsHeapStatEntry expected_heap_stats[] = {
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info =
        {
          .size = 1234,
          .in_use = 0,
        },
    },
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info =
        {
          .size = 1234,
          .in_use = 0,
        },
    },
  };

  bool empty = ticos_heap_stats_empty();
  CHECK(empty);
  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  TICOS_HEAP_STATS_FREE(expected_heap_stats[0].ptr);
  TICOS_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  TICOS_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  empty = ticos_heap_stats_empty();
  CHECK(!empty);

  size_t list_count = 0;
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    sTcsHeapStatEntry *pthis = &g_ticos_heap_stats_pool[i];
    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected_heap_stats[i], pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(1, list_count);
}

//! Verifies logic for reusing heap stat entries.
//!
//! Heap stats should first look for free entries, then overwrite the oldest entry
//! if none are free
TEST(TicosHeapStats, Test_Reuse) {
  void *lr;
  TICOS_GET_LR(lr);
  const sTcsHeapStatEntry expected_heap_stats = {
    .lr = lr,
    .ptr = (void *)0x12345679,
    .info = {
      .size = 1234,
      .in_use = 1,
    },
  };

  const sTcsHeapStatEntry middle_entry = {
    .lr = lr,
    .ptr = (void *)0x87654321,
    .info = {
      .size = 4321,
      .in_use = 1,
    },
  };

  const sTcsHeapStatEntry final_entry = {
    .lr = lr,
    .ptr = (void *)0x111111111,
    .info = {
      .size = 1111,
      .in_use = 1,
    },
  };

  // Offset to unique entry in middle of heap stats
  size_t middle_offset = TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool) >> 1;

  // Fill up the heap stats pool
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    TICOS_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + i),
                               expected_heap_stats.info.size + i);
  }

  // Free entry in middle of array, then malloc again
  TICOS_HEAP_STATS_FREE((void *)((uintptr_t)expected_heap_stats.ptr + middle_offset));
  TICOS_HEAP_STATS_MALLOC(middle_entry.ptr, middle_entry.info.size);

  // Check latest entry is list head
  LONGS_EQUAL(middle_offset, g_ticos_heap_stats.stats_pool_head);
  prv_heap_stat_equality(&middle_entry,
                         &g_ticos_heap_stats_pool[g_ticos_heap_stats.stats_pool_head]);

  // Check next recent is end of heap stats pool
  sTcsHeapStatEntry *entry = &g_ticos_heap_stats_pool[g_ticos_heap_stats.stats_pool_head];
  prv_heap_stat_equality(&g_ticos_heap_stats_pool[TICOS_HEAP_STATS_MAX_COUNT - 1],
                         &g_ticos_heap_stats_pool[entry->info.next_entry_index]);

  // Allocate another, should overwrite first entry (current oldest)
  TICOS_HEAP_STATS_MALLOC(final_entry.ptr, final_entry.info.size);
  LONGS_EQUAL(0, g_ticos_heap_stats.stats_pool_head);
  prv_heap_stat_equality(g_ticos_heap_stats_pool,
                         &g_ticos_heap_stats_pool[g_ticos_heap_stats.stats_pool_head]);

  // walk the other entries and confirm correct values
  size_t list_count = 0;
  for (size_t i = 1; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    if (i == middle_offset) {
      continue;
    }

    sTcsHeapStatEntry *pthis = &g_ticos_heap_stats_pool[i];
    sTcsHeapStatEntry expected = {
      .lr = lr,
      .ptr = (void *)(0x12345679 + i),
      .info =
        {
          .size = 1234 + (uint32_t)i,
          .in_use = 1,
        },
    };

    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected, pthis);
      if (!match) {
        fprintf(stderr, "Mismatch at index %zu\n", i);
      }
      CHECK(match);
    }
  }

  // We skipped two entries from the pool
  LONGS_EQUAL(TICOS_HEAP_STATS_MAX_COUNT - 2, list_count);
}

//! Tests handling freeing the most recent allocation (list head)
TEST(TicosHeapStats, Test_FreeMostRecent) {
  void *lr;
  TICOS_GET_LR(lr);

  const sTcsHeapStatEntry expected_heap_stats = {
    .lr = lr,
    .ptr = (void *)0x12345679,
    .info = {
      .size = 1234,
      .in_use = 1,
    },
  };

  size_t end_offset = TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool) - 1;

  // Fill up the heap stats pool
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_heap_stats_pool); i++) {
    TICOS_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + i),
                               expected_heap_stats.info.size + i);
  }

  // Remove most recent entry (last in pool)
  TICOS_HEAP_STATS_FREE((void *)((uintptr_t)expected_heap_stats.ptr + end_offset));

  // Check that most recent entry is second to last, check entry contents
  LONGS_EQUAL(end_offset - 1, g_ticos_heap_stats.stats_pool_head);
  prv_heap_stat_equality(&expected_heap_stats,
                         &g_ticos_heap_stats_pool[g_ticos_heap_stats.stats_pool_head]);
}
