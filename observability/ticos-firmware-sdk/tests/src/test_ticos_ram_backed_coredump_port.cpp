#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"

#include <string.h>
#include <stddef.h>

#include "ticos/config.h"
#include "ticos/panics/platform/coredump.h"
#include "ticos/core/compiler.h"

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  (void)start_addr;
  return desired_size;
}

TEST_GROUP(TcsRamBackedCoredumpPort) {
  void setup() { }
  void teardown() {}
};

TEST(TcsRamBackedCoredumpPort, Test_GetRegions) {
  sCoredumpCrashInfo info = {
    .stack_address = (void *)0x28000000,
  };

  size_t num_regions;
  const sTcsCoredumpRegion *regions =
      ticos_platform_coredump_get_regions(&info, &num_regions);
  LONGS_EQUAL(1, num_regions);

  const sTcsCoredumpRegion *r = &regions[0];
  LONGS_EQUAL(kTcsCoredumpRegionType_Memory, r->type);
  LONGS_EQUAL(info.stack_address, r->region_start);
  LONGS_EQUAL(TICOS_PLATFORM_ACTIVE_STACK_SIZE_TO_COLLECT, r->region_size);
}

TEST(TcsRamBackedCoredumpPort, Test_GetInfo) {
  sTcsCoredumpStorageInfo info;
  ticos_platform_coredump_storage_get_info(&info);

  // Assert if the size changes so we can catch this and make _sure_ it's what we want to do
  LONGS_EQUAL(TICOS_PLATFORM_COREDUMP_STORAGE_RAM_SIZE, info.size);
}

static size_t prv_get_size(void) {
  sTcsCoredumpStorageInfo info;
  ticos_platform_coredump_storage_get_info(&info);
  return info.size;
}

TEST(TcsRamBackedCoredumpPort, Test_BadOffsets) {
  size_t size = prv_get_size();

  uint8_t byte = 0xAB;
  bool success = ticos_platform_coredump_storage_read(size, &byte, sizeof(byte));
  CHECK(!success);
  uint32_t word = 0x12345678;
  success = ticos_platform_coredump_storage_read(size - 1, &word, sizeof(word));
  CHECK(!success);

  success = ticos_platform_coredump_storage_erase(size, size);
  CHECK(!success);
  success = ticos_platform_coredump_storage_erase(0, size + 1);
  CHECK(!success);

  success = ticos_platform_coredump_storage_write(size, &byte, sizeof(byte));
  CHECK(!success);
  success = ticos_platform_coredump_storage_write(size, &word, sizeof(word));
  CHECK(!success);
}

static void prv_assert_storage_empty(void) {
  const size_t size = prv_get_size();
  uint8_t data[size];
  const bool success = ticos_platform_coredump_storage_read(0, &data, sizeof(data));
  CHECK(success);
  uint8_t expected_data[size];
  memset(expected_data, 0x0, sizeof(expected_data));
  MEMCMP_EQUAL(expected_data, data, sizeof(data));
}

TEST(TcsRamBackedCoredumpPort, Test_EraseReadWrite) {
  const size_t size = prv_get_size();

  ticos_platform_coredump_storage_erase(0, size);
  prv_assert_storage_empty();

  // write data byte-by-byte
  for (size_t i = 0; i < size; i++) {
    uint8_t d = (i + 1) & 0xff;
    const bool success = ticos_platform_coredump_storage_write(i, &d, sizeof(d));
    CHECK(success);
  }

  // read data back byte-by-byte and confirm it matches expectations
  for (size_t i = 0; i < size; i++) {
    uint8_t d = 0;
    const bool success = ticos_platform_coredump_storage_read(i, &d, sizeof(d));
    CHECK(success);
    LONGS_EQUAL((i + 1) & 0xff, d);
  }

  // clear the coredump (needs to invalidate at least the first byte)
  ticos_platform_coredump_storage_clear();
  uint8_t clear_byte = 0xab;
  bool success = ticos_platform_coredump_storage_read(0, &clear_byte, sizeof(clear_byte));
  CHECK(success);
  LONGS_EQUAL(0, clear_byte);

  // erase everything
  ticos_platform_coredump_storage_erase(0, size);
  prv_assert_storage_empty();
}
