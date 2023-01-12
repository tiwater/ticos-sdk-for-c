#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! NOTE: The internals of the metric APIs make use of "X-Macros" to enable more flexibility
//! improving and extending the internal implementation without impacting the externally facing API

#ifdef __cplusplus
extern "C" {
#endif

#include "ticos/config.h"

//! Generate extern const char * declarations for all IDs (used in key names):
#define TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, _min, _max) \
  TICOS_METRICS_KEY_DEFINE(key_name, value_type)

#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_METRICS_KEY_DEFINE(key_name, _)

#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
  extern const char * const g_ticos_metrics_id_##key_name;
#include "ticos/metrics/heartbeat_config.def"
#include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef TICOS_METRICS_KEY_DEFINE
#undef TICOS_METRICS_KEY_DEFINE_WITH_RANGE
#undef TICOS_METRICS_STRING_KEY_DEFINE

//! Generate an enum for all IDs (used for indexing into values)
#define TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  TICOS_METRICS_KEY_DEFINE(key_name, value_type)

#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_METRICS_KEY_DEFINE(key_name, _)

#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
  kTcsMetricsIndex_##key_name,

typedef enum TcsMetricsIndex {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_KEY_DEFINE_WITH_RANGE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
} eTcsMetricsIndex;

//! Stub define to detect accidental usage outside of the heartbeat config files
#define TICOS_METRICS_KEY_DEFINE_TRAP_() \
  TICOS_STATIC_ASSERT(false, \
    "TICOS_METRICS_KEY_DEFINE should only be used in " TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE);

//! NOTE: Access to a key should _always_ be made via the TICOS_METRICS_KEY() macro to ensure source
//! code compatibility with future APIs updates
//!
//! The struct wrapper does not have any function, except for preventing one from passing a C
//! string to the API:
typedef struct {
  int _impl;
} TicosMetricId;

#define _TICOS_METRICS_ID_CREATE(id) \
  { kTcsMetricsIndex_##id }

#define _TICOS_METRICS_ID(id) \
  ((TicosMetricId) { kTcsMetricsIndex_##id })

#ifdef __cplusplus
}
#endif
