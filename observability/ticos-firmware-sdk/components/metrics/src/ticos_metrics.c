//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Ticos metrics API implementation. See header for more details.

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/event_storage_implementation.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/platform/overrides.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/metrics/metrics.h"
#include "ticos/metrics/platform/overrides.h"
#include "ticos/metrics/platform/timer.h"
#include "ticos/metrics/serializer.h"
#include "ticos/metrics/utils.h"

//! Disable this warning; it trips when there's no custom macros defined of a
//! given type
TICOS_DISABLE_WARNING("-Wunused-macros")

#undef TICOS_METRICS_KEY_DEFINE
#undef TICOS_METRICS_KEY_DEFINE_WITH_RANGE
#undef TICOS_METRICS_STRING_KEY_DEFINE

#define TICOS_METRICS_KEY_NOT_FOUND (-1)
#define TICOS_METRICS_TYPE_INCOMPATIBLE (-2)
#define TICOS_METRICS_TYPE_BAD_PARAM (-3)
#define TICOS_METRICS_TYPE_NO_CHANGE (-4)
#define TICOS_METRICS_STORAGE_TOO_SMALL (-5)
#define TICOS_METRICS_TIMER_BOOT_FAILED (-6)

typedef struct TicosMetricKVPair {
  TicosMetricId key;
  eTicosMetricType type;
  // Notes:
  // - We treat 'min' as a _signed_ integer when the 'type' == kTicosMetricType_Signed
  // - We parse this range information in the Ticos cloud to better normalize data presented in
  //   the UI.
  uint32_t min;
  uint32_t range;
} sTicosMetricKVPair;

// Generate heartbeat keys table (ROM):
#define TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, min_value, max_value) \
  { .key = _TICOS_METRICS_ID_CREATE(key_name), .type = value_type, \
    .min = (uint32_t)min_value, .range = ((int64_t)max_value - (int64_t)min_value) },

#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
  TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, 0, 0)

// Store the string max length in the range field (excluding the null terminator)
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, kTicosMetricType_String, 0, max_length)

static const sTicosMetricKVPair s_ticos_heartbeat_keys[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_KEY_DEFINE_WITH_RANGE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};

#define TICOS_METRICS_KEY_DEFINE_WITH_RANGE(key_name, value_type, _min, _max) \
  TICOS_METRICS_KEY_DEFINE(key_name, value_type)

#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_METRICS_KEY_DEFINE(key_name, value_type)

// Generate global ID constants (ROM):
#define TICOS_METRICS_KEY_DEFINE(key_name, value_type)         \
  const char * const g_ticos_metrics_id_##key_name = TICOS_EXPAND_AND_QUOTE(key_name);

#include "ticos/metrics/heartbeat_config.def"
#include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef TICOS_METRICS_KEY_DEFINE
#undef TICOS_METRICS_STRING_KEY_DEFINE

TICOS_STATIC_ASSERT(TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys) != 0,
                       "At least one \"TICOS_METRICS_KEY_DEFINE\" must be defined");

#define TICOS_METRICS_TIMER_VAL_MAX 0x80000000
typedef struct TicosMetricValueMetadata {
  bool is_running:1;
  // We'll use 32 bits since the rollover time is ~25 days which is much much greater than a
  // reasonable heartbeat interval. This let's us track whether or not the timer is running in the
  // top bit
  uint32_t start_time_ms:31;
} sTicosMetricValueMetadata;

typedef struct TicosMetricValueInfo {
  union TicosMetricValue *valuep;
  sTicosMetricValueMetadata *meta_datap;
} sTicosMetricValueInfo;


// Allocate storage for string values- additional byte for null terminator.
#define TICOS_METRICS_KEY_DEFINE(key_name, value_type)
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  static char g_ticos_metrics_string_##key_name[max_length + 1 /* for NUL */];
#include "ticos/metrics/heartbeat_config.def"
#include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
#undef TICOS_METRICS_KEY_DEFINE
#undef TICOS_METRICS_STRING_KEY_DEFINE

// Generate a mapping of key index to key value position in s_ticos_heartbeat_values.
// First produce a sparse enum for the key values that are stored in s_ticos_heartbeat_values.
typedef enum TcsMetricKeyToValueIndex {
  #define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
    kTcsMetricKeyToValueIndex_##key_name,
  #define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length)

  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
  kTcsMetricKeyToValueIndex_Count
} eTcsMetricKeyToValueIndex;
// Now generate a table mapping the canonical key ID to the index in s_ticos_heartbeat_values
static const eTcsMetricKeyToValueIndex s_ticos_heartbeat_key_to_valueindex[] = {
  #define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
    kTcsMetricKeyToValueIndex_##key_name,
  #define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    0,  // 0 for the placeholder so it's safe to index with

  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};
TICOS_STATIC_ASSERT(TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys) == TICOS_ARRAY_SIZE(s_ticos_heartbeat_key_to_valueindex),
                       "Mismatch between s_ticos_heartbeat_keys and s_ticos_heartbeat_key_to_valueindex");
// And a similar approach for the strings
typedef enum TcsMetricStringKeyToIndex {
  #define TICOS_METRICS_KEY_DEFINE(key_name, value_type)
  #define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    kTcsMetricStringKeyToIndex_##key_name,

  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
  kTcsMetricStringKeyToIndex_Count
} eTcsMetricStringKeyToIndex;
// Now generate a table mapping the canonical key ID to the index in s_ticos_heartbeat_values
static const eTcsMetricStringKeyToIndex s_ticos_heartbeat_string_key_to_index[] = {
  #define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
    (eTcsMetricStringKeyToIndex)0,  // 0 for the placeholder so it's safe to index with
  #define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
    kTcsMetricStringKeyToIndex_##key_name,

  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE

  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};
TICOS_STATIC_ASSERT(TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys) == TICOS_ARRAY_SIZE(s_ticos_heartbeat_string_key_to_index),
                       "Mismatch between s_ticos_heartbeat_keys and s_ticos_heartbeat_string_key_to_index");

// Generate heartbeat values table (RAM), sparsely populated: only for the scalar types
#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) { 0 },
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length)
static union TicosMetricValue s_ticos_heartbeat_values[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};

// String value lookup table. Const- the pointers do not change at runtime, so
// this table can be stored in ROM and save a little RAM.
#define TICOS_METRICS_KEY_DEFINE(key_name, value_type)
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) { .ptr = g_ticos_metrics_string_##key_name },
static const union TicosMetricValue s_ticos_heartbeat_string_values[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
  // include a stub entry to prevent compilation errors when no strings are defined
  {.ptr = NULL},
};

// Timer metadata table
#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Unsigned(_name)
#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Signed(_name)
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length)
#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Timer(_name) { 0 },
#define TICOS_METRICS_KEY_DEFINE(_name, _type) \
  TICOS_METRICS_STATE_HELPER_##_type(_name)
static sTicosMetricValueMetadata s_ticos_heartbeat_timer_values_metadata[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  // allocate at least one entry so we don't have an empty array in the situation
  // where no Timer metrics are defined:
  TICOS_METRICS_KEY_DEFINE(_, kTicosMetricType_Timer)
  #undef TICOS_METRICS_KEY_DEFINE
};
// Work-around for unused-macros error in case not all types are used in the .def file:
TICOS_METRICS_STATE_HELPER_kTicosMetricType_Unsigned(_)
TICOS_METRICS_STATE_HELPER_kTicosMetricType_Signed(_)

// We need a key-index table of pointers to timer metadata for fast lookups.
// The enum eTcsMetricsTimerIndex will create a subset of indexes for use
// in the s_ticos_heartbeat_timer_values_metadata[] table. The
// s_metric_timer_metadata_mapping[] table provides the mapping from the
// exhaustive list of keys to valid timer indexes or -1 if not a timer.
#define TICOS_METRICS_KEY_DEFINE(_name, _type) \
   TICOS_METRICS_STATE_HELPER_##_type(_name)

// String metrics are not present in eTcsMetricsTimerIndex
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length)

#undef TICOS_METRICS_STATE_HELPER_kTicosMetricType_Timer
#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Timer(key_name) \
  kTcsMetricsTimerIndex_##key_name,

typedef enum TcsTimerIndex {
   #include "ticos/metrics/heartbeat_config.def"
   #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
} eTcsMetricsTimerIndex;

#undef TICOS_METRICS_STATE_HELPER_kTicosMetricType_Unsigned
#undef TICOS_METRICS_STATE_HELPER_kTicosMetricType_Signed
#undef TICOS_METRICS_STRING_KEY_DEFINE

#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Unsigned(_name) \
  -1,
#define TICOS_METRICS_STATE_HELPER_kTicosMetricType_Signed(_name) \
  -1,
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  -1,

static const int s_metric_timer_metadata_mapping[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};

static struct {
  const sTicosEventStorageImpl *storage_impl;
} s_ticos_metrics_ctx;

//
// Routines which can be overridden by customers
//

TICOS_WEAK
void ticos_metrics_heartbeat_collect_data(void) { }

TICOS_WEAK
void ticos_metrics_heartbeat_collect_sdk_data(void) { }

// Returns NULL if not a timer type or out of bounds index.
static sTicosMetricValueMetadata *prv_find_timer_metadatap(eTcsMetricsIndex metric_index) {
  if (metric_index >= TICOS_ARRAY_SIZE(s_metric_timer_metadata_mapping)) {
    TICOS_LOG_ERROR("Metric index %u exceeds expected array bounds %d\n",
                       metric_index, (int)TICOS_ARRAY_SIZE(s_metric_timer_metadata_mapping));
    return NULL;
  }

  const int timer_index = s_metric_timer_metadata_mapping[metric_index];
  if (timer_index == -1) {
    return NULL;
  }

  return &s_ticos_heartbeat_timer_values_metadata[timer_index];
}

static eTicosMetricType prv_find_value_for_key(TicosMetricId key,
                                                  sTicosMetricValueInfo *value_info_out) {
  const size_t idx = (size_t)key._impl;
  if (idx >= TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys)) {
    *value_info_out = (sTicosMetricValueInfo){0};
    return kTicosMetricType_NumTypes;
  }

  // get the index for the value matching this key.
  eTcsMetricKeyToValueIndex key_index = s_ticos_heartbeat_key_to_valueindex[idx];
  // for scalar types, this will be the returned value pointer. non-scalars
  // will be handled in the switch below
  union TicosMetricValue *value_ptr = &s_ticos_heartbeat_values[key_index];

  eTicosMetricType key_type = s_ticos_heartbeat_keys[idx].type;
  switch (key_type) {
    case kTicosMetricType_String:
    {
      // get the string value associated with this key
      eTcsMetricStringKeyToIndex string_key_index = s_ticos_heartbeat_string_key_to_index[idx];
      // cast to uintptr_t then the final pointer type we want to drop the
      // 'const' and prevent tripping -Wcast-qual. this is safe, because we
      // never modify *value_ptr, only value_ptr->ptr, for non-scalar types.
      value_ptr = (union TicosMetricValue
                     *)(uintptr_t)&s_ticos_heartbeat_string_values[string_key_index];

    } break;

    case kTicosMetricType_Timer:
    case kTicosMetricType_Signed:
    case kTicosMetricType_Unsigned:
    case kTicosMetricType_NumTypes:  // To silence -Wswitch-enum
    default:
      break;
  }

  *value_info_out = (sTicosMetricValueInfo){
    .valuep = value_ptr,
    .meta_datap = prv_find_timer_metadatap((eTcsMetricsIndex)idx),
  };

  return key_type;
}

typedef bool (*TicosMetricKvIteratorCb)(void *ctx,
                                           const sTicosMetricKVPair *kv_pair,
                                           const sTicosMetricValueInfo *value_info);
static void prv_metric_iterator(void *ctx, TicosMetricKvIteratorCb cb) {
  for (uint32_t idx = 0; idx < TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys); ++idx) {
    const sTicosMetricKVPair *const kv_pair = &s_ticos_heartbeat_keys[idx];
    sTicosMetricValueInfo value_info = {0};

    (void)prv_find_value_for_key(kv_pair->key, &value_info);

    bool do_continue = cb(ctx, kv_pair, &value_info);

    if (!do_continue) {
      break;
    }
  }
}

static const sTicosMetricKVPair * prv_find_kvpair_for_key(TicosMetricId key) {
  const size_t idx = (size_t)key._impl;
  if (idx >= TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys)) {
    return NULL;
  }

  return &s_ticos_heartbeat_keys[idx];
}

static int prv_find_value_info_for_type(TicosMetricId key, eTicosMetricType expected_type,
                                        sTicosMetricValueInfo *value_info) {
  const eTicosMetricType type = prv_find_value_for_key(key, value_info);
  if (value_info->valuep == NULL) {
    return TICOS_METRICS_KEY_NOT_FOUND;
  }
  if (type != expected_type) {
    // To easily get name of metric in gdb, p/s (eTcsMetricsIndex)0
    TICOS_LOG_ERROR("Invalid type (%u vs %u) for key: %d", expected_type, type, key._impl);
    return TICOS_METRICS_TYPE_INCOMPATIBLE;
  }
  return 0;
}

static int prv_find_and_set_value_for_key(
    TicosMetricId key, eTicosMetricType expected_type, union TicosMetricValue *new_value) {
  sTicosMetricValueInfo value_info = {0};
  int rv = prv_find_value_info_for_type(key, expected_type, &value_info);
  if (rv != 0) {
    return rv;
  }

  *value_info.valuep = *new_value;
  return 0;
}

int ticos_metrics_heartbeat_set_signed(TicosMetricId key, int32_t signed_value) {
  int rv;
  ticos_lock();
  {
    rv = prv_find_and_set_value_for_key(key, kTicosMetricType_Signed,
                                        &(union TicosMetricValue){.i32 = signed_value});
  }
  ticos_unlock();
  return rv;
}

int ticos_metrics_heartbeat_set_unsigned(TicosMetricId key, uint32_t unsigned_value) {
  int rv;
  ticos_lock();
  {
    rv = prv_find_and_set_value_for_key(key, kTicosMetricType_Unsigned,
                                        &(union TicosMetricValue){.u32 = unsigned_value});
  }
  ticos_unlock();
  return rv;
}

int ticos_metrics_heartbeat_set_string(TicosMetricId key, const char *value) {
  int rv;
  ticos_lock();
  {
    sTicosMetricValueInfo value_info = {0};
    rv = prv_find_value_info_for_type(key, kTicosMetricType_String, &value_info);
    const sTicosMetricKVPair *kv = prv_find_kvpair_for_key(key);

    // error if either the key is bad, or we can't find the kvpair for the key
    // (both checks should have the same result though)
    rv = (rv != 0 || kv == NULL) ? TICOS_METRICS_KEY_NOT_FOUND : 0;

    if (rv == 0) {
      const size_t len = TICOS_MIN(strlen(value), kv->range);
      memcpy(value_info.valuep->ptr, value, len);
      // null terminate
      ((char *)value_info.valuep->ptr)[len] = '\0';
    }
  }
  ticos_unlock();
  return rv;
}

typedef enum {
  kTicosTimerOp_Start,
  kTicosTimerOp_Stop,
  kTicosTimerOp_ForceValueUpdate,
} eTicosTimerOp;

static bool prv_update_timer_metric(const sTicosMetricValueInfo *value_info,
                                    eTicosTimerOp op) {
  sTicosMetricValueMetadata *meta_datap = value_info->meta_datap;
  const bool timer_running = meta_datap->is_running;

  // The timer is not running _and_ we received a Start request
  if (!timer_running && op == kTicosTimerOp_Start) {
    meta_datap->start_time_ms = ticos_platform_get_time_since_boot_ms();
    meta_datap->is_running = true;
    return true;
  }

  // the timer is running and we received a Stop or ForceValueUpdate request
  if (timer_running && op != kTicosTimerOp_Start) {
    const uint32_t stop_time_ms =
        ticos_platform_get_time_since_boot_ms() & ~TICOS_METRICS_TIMER_VAL_MAX;
    const uint32_t start_time_ms = meta_datap->start_time_ms;

    uint32_t delta;
    if (stop_time_ms >= start_time_ms) {
      delta = stop_time_ms - start_time_ms;
    } else { // account for rollover
      delta = TICOS_METRICS_TIMER_VAL_MAX - start_time_ms + stop_time_ms;
    }
    value_info->valuep->u32 += delta;

    if (op == kTicosTimerOp_Stop) {
      meta_datap->start_time_ms = 0;
      meta_datap->is_running = false;
    } else {
      meta_datap->start_time_ms = stop_time_ms;
    }

    return true;
  }

  // We were already in the state requested and no update took place
  return false;
}

static int prv_find_timer_metric_and_update(TicosMetricId key, eTicosTimerOp op) {
  sTicosMetricValueInfo value_info = {0};
  int rv = prv_find_value_info_for_type(key, kTicosMetricType_Timer, &value_info);
  if (rv != 0) {
    return rv;
  }

  // If the value did not change because the timer was already in the state requested return an
  // error code. This will make it easier for users of the external API to catch if their calls
  // were unbalanced.
  const bool did_update = prv_update_timer_metric(&value_info, op);
  return did_update ? 0 : TICOS_METRICS_TYPE_NO_CHANGE;
}

int ticos_metrics_heartbeat_timer_start(TicosMetricId key) {
  int rv;
  ticos_lock();
  {
    rv = prv_find_timer_metric_and_update(key, kTicosTimerOp_Start);
  }
  ticos_unlock();
  return rv;
}

int ticos_metrics_heartbeat_timer_stop(TicosMetricId key) {
  int rv;
  ticos_lock();
  {
    rv = prv_find_timer_metric_and_update(key, kTicosTimerOp_Stop);
  }
  ticos_unlock();
  return rv;
}

static bool prv_tally_and_update_timer_cb(TICOS_UNUSED void *ctx,
                                          const sTicosMetricKVPair *key,
                                          const sTicosMetricValueInfo *value) {
  if (key->type != kTicosMetricType_Timer) {
    return true;
  }

  prv_update_timer_metric(value, kTicosTimerOp_ForceValueUpdate);
  return true;
}

static void prv_reset_metrics(void) {
  // reset all scalar metric values
  memset(s_ticos_heartbeat_values, 0, sizeof(s_ticos_heartbeat_values));

  // reset all string metric values. -1 to skip the last, stub entry in the
  // table
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_ticos_heartbeat_string_values); i++) {
    // set null terminator
    if (s_ticos_heartbeat_string_values[i].ptr) {
      ((char *)s_ticos_heartbeat_string_values[i].ptr)[0] = 0;
    }
  }
}

static void prv_heartbeat_timer_update(void) {
  // force an update of the timer value for any actively running timers
  prv_metric_iterator(NULL, prv_tally_and_update_timer_cb);
}

//! Trigger an update of heartbeat metrics, serialize out to storage, and reset.
static void prv_heartbeat_timer(void) {
  prv_heartbeat_timer_update();
  ticos_metrics_heartbeat_collect_sdk_data();
  ticos_metrics_heartbeat_collect_data();

  ticos_metrics_heartbeat_serialize(s_ticos_metrics_ctx.storage_impl);

  prv_reset_metrics();
}

static int prv_find_key_and_add(TicosMetricId key, int32_t amount) {
  sTicosMetricValueInfo value_info = {0};
  const eTicosMetricType type = prv_find_value_for_key(key, &value_info);
  if (value_info.valuep == NULL) {
    return TICOS_METRICS_KEY_NOT_FOUND;
  }
  union TicosMetricValue *value = value_info.valuep;

  switch ((int)type) {
    case kTicosMetricType_Signed: {
      // Clip in case of overflow:
      int64_t new_value = (int64_t)value->i32 + (int64_t)amount;
      if (new_value > INT32_MAX) {
        value->i32 = INT32_MAX;
      } else if (new_value < INT32_MIN) {
        value->i32 = INT32_MIN;
      } else {
        value->i32 = new_value;
      }
      break;
    }

    case kTicosMetricType_Unsigned: {
      uint32_t new_value = value->u32 + (uint32_t)amount;
      const bool amount_is_positive = amount > 0;
      const bool did_increase = new_value > value->u32;
      // Clip in case of overflow:
      if ((uint32_t)amount_is_positive ^ (uint32_t)did_increase) {
        new_value = amount_is_positive ? UINT32_MAX : 0;
      }
      value->u32 = new_value;
      break;
    }

    case kTicosMetricType_Timer:
    case kTicosMetricType_String:
    case kTicosMetricType_NumTypes: // To silence -Wswitch-enum
    default:
      // To easily get name of metric in gdb, p/s (eTcsMetricsIndex)0
      TICOS_LOG_ERROR("Can only add to number types (key: %d)", key._impl);
      return TICOS_METRICS_TYPE_INCOMPATIBLE;
  }
  return 0;
}

int ticos_metrics_heartbeat_add(TicosMetricId key, int32_t amount) {
  int rv;
  ticos_lock();
  {
    rv = prv_find_key_and_add(key, amount);
  }
  ticos_unlock();
  return rv;
}

static int prv_find_key_of_type(TicosMetricId key, eTicosMetricType expected_type,
                                union TicosMetricValue **value_out) {
  sTicosMetricValueInfo value_info = {0};
  const eTicosMetricType type = prv_find_value_for_key(key, &value_info);
  if (value_info.valuep == NULL) {
    return TICOS_METRICS_KEY_NOT_FOUND;
  }
  if (type != expected_type) {
    return TICOS_METRICS_TYPE_INCOMPATIBLE;
  }
  *value_out = value_info.valuep;
  return 0;
}

int ticos_metrics_heartbeat_read_unsigned(TicosMetricId key, uint32_t *read_val) {
  if (read_val == NULL) {
    return TICOS_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  ticos_lock();
  {
    union TicosMetricValue *value;
    rv = prv_find_key_of_type(key, kTicosMetricType_Unsigned, &value);
    if (rv == 0) {
      *read_val = value->u32;
    }
  }
  ticos_unlock();
  return rv;
}

int ticos_metrics_heartbeat_read_signed(TicosMetricId key, int32_t *read_val) {
  if (read_val == NULL) {
    return TICOS_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  ticos_lock();
  {
    union TicosMetricValue *value;
    rv = prv_find_key_of_type(key, kTicosMetricType_Signed, &value);
    if (rv == 0) {
      *read_val = value->i32;
    }
  }
  ticos_unlock();
  return rv;
}

int ticos_metrics_heartbeat_timer_read(TicosMetricId key, uint32_t *read_val) {
  if (read_val == NULL) {
    return TICOS_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  ticos_lock();
  {
    union TicosMetricValue *value;
    rv = prv_find_key_of_type(key, kTicosMetricType_Timer, &value);
    if (rv == 0) {
      *read_val = value->u32;
    }
  }
  ticos_unlock();
  return rv;
}
int ticos_metrics_heartbeat_read_string(TicosMetricId key, char *read_val,
                                           size_t read_val_len) {
  if ((read_val == NULL) || (read_val_len == 0)) {
    return TICOS_METRICS_TYPE_BAD_PARAM;
  }

  int rv;
  ticos_lock();
  {
    union TicosMetricValue *value;
    rv = prv_find_key_of_type(key, kTicosMetricType_String, &value);
    const sTicosMetricKVPair *kv = prv_find_kvpair_for_key(key);

    rv = (rv != 0 || kv == NULL) ? TICOS_METRICS_KEY_NOT_FOUND : 0;

    if (rv == 0) {
      // copy up to the min of the length of the string and the length of the
      // provided buffer
      size_t len = strlen(value->ptr) + 1;
      memcpy(read_val, value->ptr, TICOS_MIN(len, read_val_len));
      // always null terminate
      read_val[read_val_len - 1] = '\0';
    }
  }
  ticos_unlock();

  return rv;
}

typedef struct {
  TicosMetricIteratorCallback user_cb;
  void *user_ctx;
} sMetricHeartbeatIterateCtx;

static bool prv_metrics_heartbeat_iterate_cb(void *ctx,
                                             const sTicosMetricKVPair *key_info,
                                             const sTicosMetricValueInfo *value_info) {
  sMetricHeartbeatIterateCtx *ctx_info = (sMetricHeartbeatIterateCtx *)ctx;

  sTicosMetricInfo info = {
    .key = key_info->key,
    .type = key_info->type,
    .val = *value_info->valuep
  };
  return ctx_info->user_cb(ctx_info->user_ctx, &info);
}

void ticos_metrics_heartbeat_iterate(TicosMetricIteratorCallback cb, void *ctx) {
  ticos_lock();
  {
    sMetricHeartbeatIterateCtx user_ctx = {
      .user_cb = cb,
      .user_ctx = ctx,
    };
    prv_metric_iterator(&user_ctx, prv_metrics_heartbeat_iterate_cb);
  }
  ticos_unlock();
}

size_t ticos_metrics_heartbeat_get_num_metrics(void) {
  return TICOS_ARRAY_SIZE(s_ticos_heartbeat_keys);
}

#define TICOS_METRICS_KEY_DEFINE(key_name, value_type) \
  TICOS_QUOTE(key_name),
#define TICOS_METRICS_STRING_KEY_DEFINE(key_name, max_length) \
  TICOS_QUOTE(key_name),

static const char *s_idx_to_metric_name[] = {
  #include "ticos/metrics/heartbeat_config.def"
  #include TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE
  #undef TICOS_METRICS_KEY_DEFINE
  #undef TICOS_METRICS_STRING_KEY_DEFINE
};

static bool prv_heartbeat_debug_print(TICOS_UNUSED void *ctx,
                                      const sTicosMetricInfo *metric_info) {
  const TicosMetricId *key = &metric_info->key;
  const union TicosMetricValue *value = &metric_info->val;

  const char *key_name = s_idx_to_metric_name[key->_impl];

  switch (metric_info->type) {
    case kTicosMetricType_Unsigned:
    case kTicosMetricType_Timer:
      TICOS_LOG_DEBUG("  %s: %" PRIu32, key_name, value->u32);
      break;
    case kTicosMetricType_Signed:
      TICOS_LOG_DEBUG("  %s: %" PRIi32, key_name, value->i32);
      break;
    case kTicosMetricType_String:
      TICOS_LOG_DEBUG("  %s: \"%s\"", key_name, (const char *)value->ptr);
      break;

    case kTicosMetricType_NumTypes: // To silence -Wswitch-enum
    default:
      TICOS_LOG_DEBUG("  %s: <unknown type>", key_name);
      break;
  }

  return true; // continue iterating
}

void ticos_metrics_heartbeat_debug_print(void) {
  prv_heartbeat_timer_update();
  TICOS_LOG_DEBUG("Heartbeat keys/values:");
  ticos_metrics_heartbeat_iterate(prv_heartbeat_debug_print, NULL);
}

void ticos_metrics_heartbeat_debug_trigger(void) {
  prv_heartbeat_timer();
}

static int prv_init_unexpected_reboot_metric(void) {
  bool unexpected_reboot = false;
  int rv = ticos_reboot_tracking_get_unexpected_reboot_occurred(&unexpected_reboot);
  if (rv != 0) {
    TICOS_LOG_ERROR("Invalid reset reason read");
    return -1;
  }

  return ticos_metrics_heartbeat_set_unsigned(
    TICOS_METRICS_KEY(TicosSdkMetric_UnexpectedRebootDidOccur), unexpected_reboot ? 1 : 0);
}

int ticos_metrics_boot(const sTicosEventStorageImpl *storage_impl,
                          const sTicosMetricBootInfo *info) {
  if (storage_impl == NULL || info == NULL) {
    return TICOS_METRICS_TYPE_BAD_PARAM;
  }

  s_ticos_metrics_ctx.storage_impl = storage_impl;
  prv_reset_metrics();

  const bool success = ticos_platform_metrics_timer_boot(
      TICOS_METRICS_HEARTBEAT_INTERVAL_SECS, prv_heartbeat_timer);
  if (!success) {
    return TICOS_METRICS_TIMER_BOOT_FAILED;
  }

  if (!ticos_serializer_helper_check_storage_size(
      storage_impl, ticos_metrics_heartbeat_compute_worst_case_storage_size, "metrics")) {
    return TICOS_METRICS_STORAGE_TOO_SMALL;
  }

  int rv = ticos_metrics_heartbeat_timer_start(
      TICOS_METRICS_KEY(TicosSdkMetric_IntervalMs));
  if (rv != 0) {
    return rv;
  }

  rv = ticos_metrics_heartbeat_set_unsigned(
      TICOS_METRICS_KEY(TicosSdkMetric_UnexpectedRebootCount), info->unexpected_reboot_count);
  if (rv != 0) {
    return rv;
  }

  rv = prv_init_unexpected_reboot_metric();
  if (rv != 0) {
    return rv;
  }

  return 0;
}
