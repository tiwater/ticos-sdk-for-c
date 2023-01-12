//! @file
//!

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>
#include <stdlib.h>

#include "ticos_test_compact_log_c.h"

TEST_GROUP(TcsCompactLogMacros) {
  void setup() {
    mock().strictOrder();
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(TcsCompactLogMacros, Test_TcsCompactLog_ArgExpansion) {
  test_compact_log_c_argument_expansion();
}

TEST(TcsCompactLogMacros, Test_TcsCompactLog_MultiArg) {
  test_compact_log_c_multi_arg();
}

TEST(TcsCompactLogMacros, Test_TcsCompactLog_Complex) {
  test_compact_log_c_multi_complex();
}
