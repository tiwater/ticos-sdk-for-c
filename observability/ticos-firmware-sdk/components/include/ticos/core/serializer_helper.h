#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal helper functions that are used when serializing Ticos Event based data
//! A user of the sdk should never have to call these routines directly.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/core/event_storage.h"
#include "ticos/core/platform/system_time.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/util/cbor.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ticos_serializer_helper_encode_metadata_with_time(sTicosCborEncoder *encoder,
                                                          eTicosEventType type,
                                                          const sTicosCurrentTime *time);

  bool ticos_serializer_helper_encode_metadata(sTicosCborEncoder *encoder, eTicosEventType type);

bool ticos_serializer_helper_encode_uint32_kv_pair(
    sTicosCborEncoder *encoder, uint32_t key, uint32_t value);

bool ticos_serializer_helper_encode_int32_kv_pair(
    sTicosCborEncoder *encoder, uint32_t key, int32_t value);

bool ticos_serializer_helper_encode_byte_string_kv_pair(
  sTicosCborEncoder *encoder, uint32_t key, const void *buf, size_t buf_len);

typedef struct TicosTraceEventHelperInfo {
  eTicosTraceInfoEventKey reason_key;
  uint32_t reason_value;
  uint32_t pc;
  uint32_t lr;
  size_t extra_event_info_pairs;
} sTicosTraceEventHelperInfo;

bool ticos_serializer_helper_encode_trace_event(sTicosCborEncoder *e, const sTicosTraceEventHelperInfo *info);

//! @return false if encoding was not successful and the write session needs to be rolled back.
typedef bool (TicosSerializerHelperEncodeCallback)(sTicosCborEncoder *encoder, void *ctx);

//! Helper to initialize a CBOR encoder, prepare the storage for writing, call the encoder_callback
//! to encode and write any data and finally commit the write to the storage (or rollback in case
//! of an error).
//! @return the value returned from encode_callback
bool ticos_serializer_helper_encode_to_storage(sTicosCborEncoder *encoder,
    const sTicosEventStorageImpl *storage_impl,
    TicosSerializerHelperEncodeCallback encode_callback, void *ctx);

//! Helper to compute the size of encoding operations performed by encode_callback.
//! @return the computed size required to store the encoded data.
size_t ticos_serializer_helper_compute_size(
    sTicosCborEncoder *encoder, TicosSerializerHelperEncodeCallback encode_callback, void *ctx);

bool ticos_serializer_helper_check_storage_size(
    const sTicosEventStorageImpl *storage_impl, size_t (compute_worst_case_size)(void), const char *event_type);

//! Return the number of events that were dropped since last call
//!
//! @note Calling this function resets the counters.
uint32_t ticos_serializer_helper_read_drop_count(void);

#ifdef __cplusplus
}
#endif
