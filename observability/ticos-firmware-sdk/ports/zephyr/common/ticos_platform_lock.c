//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Wire up Zephyr locks to the Ticos mutex API

#include "ticos/core/platform/core.h"

#include <kernel.h>

K_MUTEX_DEFINE(s_ticos_mutex);

void ticos_lock(void) {
  k_mutex_lock(&s_ticos_mutex, K_FOREVER);
}

void ticos_unlock(void) {
  k_mutex_unlock(&s_ticos_mutex);
}
