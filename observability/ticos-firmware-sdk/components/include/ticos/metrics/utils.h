#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Utilities to assist with querying Ticos Metric info
//!
//! @note A user of the Ticos SDK should _never_ call any
//! of these routines directly

#include "ticos/metrics/metrics.h"

#ifdef __cplusplus
extern "C" {
#endif

union TicosMetricValue {
  uint32_t u32;
  int32_t i32;
  void *ptr;
};

typedef struct {
  TicosMetricId key;
  eTicosMetricType type;
  union TicosMetricValue val;
} sTicosMetricInfo;

//! The callback invoked when "ticos_metrics_heartbeat_iterate" is called
//!
//! @param ctx The context provided to "ticos_metrics_heartbeat_iterate"
//! @param metric_info Info for the particular metric being returned in the callback
//! See struct for more details
//!
//! @return bool to continue iterating, else false
typedef bool (*TicosMetricIteratorCallback)(void *ctx, const sTicosMetricInfo *metric_info);

void ticos_metrics_heartbeat_iterate(TicosMetricIteratorCallback cb, void *ctx);

//! @return the number of metrics being required
size_t ticos_metrics_heartbeat_get_num_metrics(void);

#ifdef __cplusplus
}
#endif
