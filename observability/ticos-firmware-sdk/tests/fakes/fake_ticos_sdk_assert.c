//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <assert.h>

#include "ticos/core/sdk_assert.h"
#include "ticos/core/compiler.h"

TICOS_NORETURN
void ticos_sdk_assert_func(void) {
  assert(0);
}
