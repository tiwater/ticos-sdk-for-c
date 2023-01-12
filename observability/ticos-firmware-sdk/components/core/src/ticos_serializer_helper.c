//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "ticos/core/serializer_helper.h"

#include <inttypes.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/event_storage_implementation.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/platform/system_time.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/util/cbor.h"

#if TICOS_EVENT_INCLUDE_BUILD_ID
#include "ticos/core/build_info.h"
#include "ticos_build_id_private.h"
#endif

typedef struct TicosSerializerOptions {
  // By default, the device serial number is not encoded in each event to conserve space
  // and instead is derived from the identifier provided when posting to the chunks endpoint
  //  (api/v0/chunks/{{device_identifier}})
  //
  // To instead always encode the device serial number, compile the Ticos SDK with the following
  // CFLAG:
  //   TICOS_EVENT_INCLUDE_DEVICE_SERIAL=0
  bool encode_device_serial;
} sTicosSerializerOptions;

//! The number of messages dropped since the last successful send
static uint32_t s_num_storage_drops = 0;
//! A running sum of total messages dropped since ticos_serializer_helper_read_drop_count() was
//! last called
static uint32_t s_last_drop_count = 0;

static const sTicosSerializerOptions s_ticos_serializer_options = {
  .encode_device_serial = (TICOS_EVENT_INCLUDE_DEVICE_SERIAL != 0),
};

static bool prv_encode_event_key_string_pair(
    sTicosCborEncoder *encoder, eTicosEventKey key,  const char *value) {
  return ticos_cbor_encode_unsigned_integer(encoder, key) &&
      ticos_cbor_encode_string(encoder, value);
}

static bool prv_encode_device_version_info(sTicosCborEncoder *e) {
  // Encoding something like:
  //
  // (Optional) "device_serial": "ABCD1234",
  // "software_type": "main-fw",
  // "software_version": "1.0.0",
  // "hardware_version": "hwrev1",
  //
  // NOTE: int keys are used instead of strings to minimize the wire payload.

  sTicosDeviceInfo info = { 0 };
  ticos_platform_get_device_info(&info);

  if (s_ticos_serializer_options.encode_device_serial &&
      !prv_encode_event_key_string_pair(e, kTicosEventKey_DeviceSerial, info.device_serial)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kTicosEventKey_SoftwareType, info.software_type)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kTicosEventKey_SoftwareVersion, info.software_version)) {
    return false;
  }

  if (!prv_encode_event_key_string_pair(e, kTicosEventKey_HardwareVersion, info.hardware_version)) {
    return false;
  }

  return true;
}

bool ticos_serializer_helper_encode_uint32_kv_pair(
    sTicosCborEncoder *encoder, uint32_t key, uint32_t value) {
  return ticos_cbor_encode_unsigned_integer(encoder, key) &&
      ticos_cbor_encode_unsigned_integer(encoder, value);
}

bool ticos_serializer_helper_encode_int32_kv_pair(
    sTicosCborEncoder *encoder, uint32_t key, int32_t value) {
  return ticos_cbor_encode_unsigned_integer(encoder, key) &&
      ticos_cbor_encode_signed_integer(encoder, value);
}

bool ticos_serializer_helper_encode_byte_string_kv_pair(
  sTicosCborEncoder *encoder, uint32_t key, const void *buf, size_t buf_len) {
  return ticos_cbor_encode_unsigned_integer(encoder, key) &&
      ticos_cbor_encode_byte_string(encoder, buf, buf_len);
}

static bool prv_encode_event_key_uint32_pair(
    sTicosCborEncoder *encoder, eTicosEventKey key, uint32_t value) {
  return ticos_cbor_encode_unsigned_integer(encoder, key) &&
         ticos_cbor_encode_unsigned_integer(encoder, value);
}

bool ticos_serializer_helper_encode_metadata(sTicosCborEncoder *encoder,
                                                eTicosEventType type) {
  sTicosCurrentTime time;
  if (!ticos_platform_time_get_current(&time)) {
    time.type = kTicosCurrentTimeType_Unknown;
  }
  return ticos_serializer_helper_encode_metadata_with_time(encoder, type, &time);
}

bool ticos_serializer_helper_encode_metadata_with_time(sTicosCborEncoder *encoder,
                                                          eTicosEventType type,
                                                          const sTicosCurrentTime *time) {
  const bool unix_timestamp_available = (time != NULL) &&
      (time->type == kTicosCurrentTimeType_UnixEpochTimeSec);

#if TICOS_EVENT_INCLUDE_BUILD_ID
  sTicosBuildInfo info;
  const bool has_build_id = ticos_build_info_read(&info);
#else
  const bool has_build_id = false;
#endif

  const size_t top_level_num_pairs =
      1 /* type */ +
      (unix_timestamp_available ? 1 : 0) +
      (s_ticos_serializer_options.encode_device_serial ? 1 : 0) +
      3 /* sw version, sw type, hw version */ +
      (has_build_id ? 1 : 0) +
      1 /* cbor schema version */ +
      1 /* event_info */;

  ticos_cbor_encode_dictionary_begin(encoder, top_level_num_pairs);


  if (!prv_encode_event_key_uint32_pair(encoder, kTicosEventKey_Type, type)) {
    return false;
  }

  if (!ticos_serializer_helper_encode_uint32_kv_pair(
          encoder, kTicosEventKey_CborSchemaVersion, TICOS_CBOR_SCHEMA_VERSION_V1)) {
    return false;
  }

  if (!prv_encode_device_version_info(encoder)) {
    return false;
  }

#if TICOS_EVENT_INCLUDE_BUILD_ID
  TICOS_STATIC_ASSERT(TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES >= 5 &&
                         TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES <= sizeof(info.build_id),
                         "TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES must be between 5 and 20 (inclusive)");
  if (has_build_id &&
      !ticos_serializer_helper_encode_byte_string_kv_pair(encoder, kTicosEventKey_BuildId, info.build_id,
                                                             TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES)) {
    return false;
  }
#endif

  return !unix_timestamp_available || prv_encode_event_key_uint32_pair(
          encoder, kTicosEventKey_CapturedDateUnixTimestamp,
          (uint32_t)time->info.unix_timestamp_secs);
}

bool ticos_serializer_helper_encode_trace_event(sTicosCborEncoder *e,
                                                   const sTicosTraceEventHelperInfo *info) {
  if (!ticos_serializer_helper_encode_metadata(e, kTicosEventType_Trace)) {
    return false;
  }

  const size_t num_entries = 1 /* reason */ +
      ((info->pc != 0) ? 1 : 0) +
      ((info->lr != 0) ? 1 : 0) +
      info->extra_event_info_pairs;

  if (!ticos_cbor_encode_unsigned_integer(e, kTicosEventKey_EventInfo) ||
      !ticos_cbor_encode_dictionary_begin(e, num_entries)) {
    return false;
  }

  if (!ticos_serializer_helper_encode_uint32_kv_pair(
      e, info->reason_key, info->reason_value)) {
    return false;
  }

  bool success = true;
  if (info->pc) {
    success = ticos_serializer_helper_encode_uint32_kv_pair(e,
        kTicosTraceInfoEventKey_ProgramCounter, info->pc);
  }

  if (success && info->lr) {
    success = ticos_serializer_helper_encode_uint32_kv_pair(e,
        kTicosTraceInfoEventKey_LinkRegister, info->lr);
  }

  return success;
}

typedef struct {
  const sTicosEventStorageImpl *storage_impl;
} sTicosSerializerHelperEncoderCtx;

static void prv_encoder_write_cb(void *ctx, TICOS_UNUSED uint32_t offset, const void *buf, size_t buf_len) {
  const sTicosEventStorageImpl *storage_impl = ((sTicosSerializerHelperEncoderCtx *) ctx)->storage_impl;
  storage_impl->append_data_cb(buf, buf_len);
}

bool ticos_serializer_helper_encode_to_storage(sTicosCborEncoder *encoder,
    const sTicosEventStorageImpl *storage_impl,
    TicosSerializerHelperEncodeCallback encode_callback, void *ctx) {
  const size_t space_available = storage_impl->begin_write_cb();
  bool success;
  {
    sTicosSerializerHelperEncoderCtx encoder_ctx = {
        .storage_impl = storage_impl,
    };
    ticos_cbor_encoder_init(encoder, prv_encoder_write_cb, &encoder_ctx, space_available);
    success = encode_callback(encoder, ctx);
    ticos_cbor_encoder_deinit(encoder);
  }
  const bool rollback = !success;
  storage_impl->finish_write_cb(rollback);

  if (!success) {
    if (s_num_storage_drops == 0) {
      TICOS_LOG_ERROR("Event storage full");
    }
    s_num_storage_drops++;
  } else if (s_num_storage_drops != 0) {
    TICOS_LOG_INFO("Event saved successfully after %d drops",
                      (int)s_num_storage_drops);
    s_last_drop_count += s_num_storage_drops;
    s_num_storage_drops = 0;
  }

  return success;
}

uint32_t ticos_serializer_helper_read_drop_count(void) {
  const uint32_t drop_count = s_last_drop_count + s_num_storage_drops;
  s_last_drop_count = 0;
  s_num_storage_drops = 0;
  return drop_count;
}

size_t ticos_serializer_helper_compute_size(sTicosCborEncoder *encoder,
    TicosSerializerHelperEncodeCallback encode_callback, void *ctx) {
  ticos_cbor_encoder_size_only_init(encoder);
  encode_callback(encoder, ctx);
  return ticos_cbor_encoder_deinit(encoder);
}

bool ticos_serializer_helper_check_storage_size(
    const sTicosEventStorageImpl *storage_impl, size_t (compute_worst_case_size)(void), const char *event_type) {
  // Check to see if the backing storage can hold at least one event
  // and return an error code in this situation so it's easier for an end user to catch it:
  const size_t storage_max_size = storage_impl->get_storage_size_cb();
  const size_t worst_case_size_needed = compute_worst_case_size();
  if (worst_case_size_needed > storage_max_size) {
    TICOS_LOG_WARN("Event storage (%d) smaller than largest %s event (%d)",
                      (int)storage_max_size, event_type, (int)worst_case_size_needed);
    return false;
  }
  return true;
}
