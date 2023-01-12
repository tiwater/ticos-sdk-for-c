//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Implements sTicosDataSourceImpl API specified in data_packetizer_source.h to serialize a
//! custom data recording such that it can be published to the Ticos cloud.

#include "ticos/core/custom_data_recording.h"
#include "ticos_custom_data_recording_private.h"

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/sdk_assert.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/util/cbor.h"

#if TICOS_CDR_ENABLE

// Holds CDR metadata -- we pre-serialize in prv_has_cdr to avoid an egregious amount of calls to
// read out the metadata when a small chunk size is used by a platform
typedef struct {
  size_t length;
  uint8_t data[TICOS_CDR_MAX_ENCODED_METADATA_LEN];
} sTicosCdrEncodedMetadata;

static const sTicosCdrSourceImpl *s_cdr_sources[TICOS_CDR_MAX_DATA_SOURCES];

typedef struct {
  sTicosCborEncoder encoder;
  const sTicosCdrSourceImpl *active_source;
  size_t total_encode_len;
  sTicosCdrMetadata active_metadata;
  sTicosCdrEncodedMetadata encoded_metadata;
} sTcsCdrSourceCtx;

static sTcsCdrSourceCtx s_ticos_cdr_source_ctx;

static bool prv_encode_cdr_metadata(sTicosCborEncoder *encoder, sTcsCdrSourceCtx *cdr_ctx) {
  const sTicosCdrMetadata *metadata = &cdr_ctx->active_metadata;

  if (!ticos_serializer_helper_encode_metadata_with_time(
          encoder, kTicosEventType_Cdr, &metadata->start_time)) {
    return false;
  }
  if (!ticos_cbor_encode_unsigned_integer(encoder, kTicosEventKey_EventInfo)) {
    return false;
  }

  const size_t cdr_num_pairs =
      1 /* mime types array */ +
      1 /* duration ms */ +
      1 /* collection reason */ +
      1 /* recording itself */;

  ticos_cbor_encode_dictionary_begin(encoder, cdr_num_pairs);

  if (!ticos_serializer_helper_encode_uint32_kv_pair(
          encoder, kTicosCdrInfoKey_DurationMs, metadata->duration_ms)) {
    return false;
  }

  if (!ticos_cbor_encode_unsigned_integer(encoder, kTicosCdrInfoKey_Mimetypes) ||
      !ticos_cbor_encode_array_begin(encoder, metadata->num_mimetypes)) {
    return false;
  }

  for (size_t i = 0; i < metadata->num_mimetypes; i++) {
    if (!ticos_cbor_encode_string(encoder, metadata->mimetypes[i])) {
      return false;
    }
  }

  if (!ticos_cbor_encode_unsigned_integer(encoder, kTicosCdrInfoKey_Reason) ||
      !ticos_cbor_encode_string(encoder, metadata->collection_reason)) {
    return false;
  }

  if (!ticos_cbor_encode_unsigned_integer(encoder, kTicosCdrInfoKey_Data)) {
    return false;
  }

  if (!ticos_cbor_encode_byte_string_begin(encoder, metadata->data_size_bytes)) {
    return false;
  }

  // Note: at this point all that's left to encode is the binary blob itself as a byte string
  return true;
}

static void prv_try_get_cdr_source_with_data(sTcsCdrSourceCtx *ctx) {
  if (ctx->active_source != NULL) {
    // a source is already active
    return;
  }

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_cdr_sources); i++) {
    const sTicosCdrSourceImpl *source = s_cdr_sources[i];
    if (source == NULL) {
      continue;
    }

    if (source->has_cdr_cb(&ctx->active_metadata)) {
      ctx->active_source = source;
      return;
    }
  }
}

static void prv_fill_header_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  uint8_t *header_buf = (uint8_t *)ctx;
  memcpy(&header_buf[offset], buf, buf_len);
}

static bool prv_has_cdr(size_t *total_size) {
  sTcsCdrSourceCtx *cdr_ctx = &s_ticos_cdr_source_ctx;

  prv_try_get_cdr_source_with_data(cdr_ctx);
  if (cdr_ctx->active_source == NULL) {
    return false;
  }

  sTicosCborEncoder encoder;
  ticos_cbor_encoder_init(&encoder, prv_fill_header_cb, cdr_ctx->encoded_metadata.data,
                             sizeof(cdr_ctx->encoded_metadata.data));

  if (!prv_encode_cdr_metadata(&encoder, cdr_ctx)) {
    TICOS_LOG_ERROR("Not enough storage to serialized CDR, increase TICOS_CDR_MAX_ENCODED_METADATA_LEN");
    return false;
  }

  cdr_ctx->encoded_metadata.length = ticos_cbor_encoder_deinit(&encoder);
  cdr_ctx->total_encode_len = cdr_ctx->encoded_metadata.length + cdr_ctx->active_metadata.data_size_bytes;
  *total_size = cdr_ctx->total_encode_len;
  return true;
}

static bool prv_cdr_read(uint32_t offset, void *buf, size_t buf_len) {
  sTcsCdrSourceCtx *cdr_ctx = &s_ticos_cdr_source_ctx;
  if (cdr_ctx->active_source == NULL) {
    return false;
  }

  if ((offset + buf_len) > cdr_ctx->total_encode_len) {
    return false;
  }

  uint8_t *bufp = (uint8_t *)buf;
  if (offset < cdr_ctx->encoded_metadata.length) {
    const size_t metadata_bytes_to_copy = TICOS_MIN(
        buf_len, cdr_ctx->encoded_metadata.length - offset);
    memcpy(bufp, &cdr_ctx->encoded_metadata.data[offset], metadata_bytes_to_copy);
    buf_len -= metadata_bytes_to_copy;

    if (buf_len == 0) {
      return true;
    }

    offset = 0;
    bufp += metadata_bytes_to_copy;
  } else {
    offset -= cdr_ctx->encoded_metadata.length;
  }

  if (!cdr_ctx->active_source->read_data_cb(offset, bufp, buf_len)) {
    return false;
  }

  return true;
}

static void prv_cdr_mark_sent(void) {
  sTcsCdrSourceCtx *cdr_ctx = &s_ticos_cdr_source_ctx;
  if (cdr_ctx->active_source == NULL) {
    return;
  }

  cdr_ctx->active_source->mark_cdr_read_cb();

  *cdr_ctx = (sTcsCdrSourceCtx) { 0 };
}

bool ticos_cdr_register_source(const sTicosCdrSourceImpl *impl) {
  // it is a configuration error if all the required dependencies are not implemented!
  TICOS_SDK_ASSERT(
      (impl != NULL) &&
      (impl->has_cdr_cb != NULL) &&
      (impl->read_data_cb != NULL) &&
      (impl->mark_cdr_read_cb != NULL));

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_cdr_sources); i++) {
    if (s_cdr_sources[i] == NULL) {
      s_cdr_sources[i] = impl;
      return true;
    }
  }

  TICOS_LOG_ERROR("Ticos Cdr Register is full, %d entries", (int)TICOS_ARRAY_SIZE(s_cdr_sources));
  return false;
}

void ticos_cdr_source_reset(void) {
  memset(s_cdr_sources, 0x0, sizeof(s_cdr_sources));
  s_ticos_cdr_source_ctx = (sTcsCdrSourceCtx) { 0x0 };
}

//! Expose a data source for use by the Ticos Packetizer
const sTicosDataSourceImpl g_ticos_cdr_source  = {
  .has_more_msgs_cb = prv_has_cdr,
  .read_msg_cb = prv_cdr_read,
  .mark_msg_read_cb = prv_cdr_mark_sent,
};

#endif /* TICOS_CDR_ENABLE */
