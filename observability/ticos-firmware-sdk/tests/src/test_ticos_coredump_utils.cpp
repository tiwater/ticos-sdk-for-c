//! @file
//!

#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "ticos/panics/coredump.h"

static sTcsCoredumpStorageInfo s_fake_coredump_info;

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  *info = s_fake_coredump_info;
}

size_t ticos_coredump_storage_compute_size_required(void) {
  return 10;
}


TEST_GROUP(TcsCoredumpUtilTestGroup) {
  void setup() {
  }

  void teardown() {
    memset(&s_fake_coredump_info, 0x0, sizeof(s_fake_coredump_info));
  }
};


TEST(TcsCoredumpUtilTestGroup, Test_TcsCoredumpUtilSizeCheck) {
  bool check_passed = ticos_coredump_storage_check_size();
  CHECK(!check_passed);

  s_fake_coredump_info.size = ticos_coredump_storage_compute_size_required() - 1;
  check_passed = ticos_coredump_storage_check_size();
  CHECK(!check_passed);

  s_fake_coredump_info.size++;
  check_passed = ticos_coredump_storage_check_size();
  CHECK(check_passed);
}
