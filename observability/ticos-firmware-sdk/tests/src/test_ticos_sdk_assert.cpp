#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <setjmp.h>
#include <stddef.h>
#include <string.h>

static jmp_buf s_assert_jmp_buf;

#include "ticos/core/platform/core.h"
#include "ticos/core/sdk_assert.h"

void ticos_platform_halt_if_debugging(void) {
  mock().actualCall(__func__);
}

void ticos_sdk_assert_func_noreturn(void) {
  // we make use of longjmp because this is a noreturn function
  longjmp(s_assert_jmp_buf, -1);
}

TEST_GROUP(TcsSdkAssert) {
  void setup() { }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(TcsSdkAssert, Test_TcsCircularBufferInit) {
  mock().expectOneCall("ticos_platform_halt_if_debugging");
  if (setjmp(s_assert_jmp_buf) == 0) {
    ticos_sdk_assert_func();
  }
}
