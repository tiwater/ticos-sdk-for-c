//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of ticos_metrics_platform_locking APIs

#include "ticos/metrics/platform/overrides.h"

#include <stdint.h>

typedef struct {
  uint32_t lock_count;
  uint32_t unlock_count;
} sMetricLockStats;

static sMetricLockStats s_metric_lock_stats;

void ticos_lock(void) {
  s_metric_lock_stats.lock_count++;
}

void ticos_unlock(void) {
  s_metric_lock_stats.unlock_count++;
}

void fake_ticos_metrics_platorm_locking_reboot(void) {
    s_metric_lock_stats = (sMetricLockStats) { 0 };
}

bool fake_ticos_platform_metrics_lock_calls_balanced(void) {
  return s_metric_lock_stats.lock_count == s_metric_lock_stats.unlock_count;
}
