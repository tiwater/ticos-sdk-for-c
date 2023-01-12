//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/core/sdk_assert.h"

#include <inttypes.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/platform/core.h"


TICOS_WEAK
void ticos_sdk_assert_func_noreturn(void) {
  while (1) { }
}

void ticos_sdk_assert_func(void) {
  void *return_address;
  TICOS_GET_LR(return_address);

  TICOS_LOG_ERROR("ASSERT! LR: 0x%" PRIx32, (uint32_t)(uintptr_t)return_address);
  ticos_platform_halt_if_debugging();
  ticos_sdk_assert_func_noreturn();
}
