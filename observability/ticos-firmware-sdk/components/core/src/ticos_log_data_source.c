//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!

#include "ticos/config.h"

#if TICOS_LOG_DATA_SOURCE_ENABLED

#include "ticos_log_data_source_private.h"
#include "ticos_log_private.h"

#include <stdint.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/overrides.h"
#include "ticos/core/platform/system_time.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/util/cbor.h"

typedef struct {
  bool triggered;
  size_t num_logs;
  sTicosCurrentTime trigger_time;
} sTcsLogDataSourceCtx;

static sTcsLogDataSourceCtx s_ticos_log_data_source_ctx;

static bool prv_log_is_sent(uint8_t hdr) {
  return hdr & TICOS_LOG_HDR_SENT_MASK;
}

typedef struct {
  size_t num_logs;
} sTcsLogCountingCtx;

static bool prv_log_iterate_counting_callback(sTcsLogIterator *iter) {
  sTcsLogCountingCtx *const ctx = (sTcsLogCountingCtx *)(iter->user_ctx);
  if (!prv_log_is_sent(iter->entry.hdr)) {
    ++ctx->num_logs;
  }
  return true;
}

void ticos_log_trigger_collection(void) {
  if (s_ticos_log_data_source_ctx.triggered) {
    return;
  }

  sTcsLogCountingCtx ctx = { 0 };
  sTcsLogIterator iter = { .user_ctx = &ctx };
  ticos_log_iterate(prv_log_iterate_counting_callback, &iter);
  if (ctx.num_logs == 0) {
    return;
  }

  ticos_lock();
  {
    // Check again in the unlikely case this function was called concurrently:
    if (s_ticos_log_data_source_ctx.triggered) {
      ticos_unlock();
      return;
    }
    s_ticos_log_data_source_ctx.triggered = true;
    if (!ticos_platform_time_get_current(&s_ticos_log_data_source_ctx.trigger_time)) {
      s_ticos_log_data_source_ctx.trigger_time.type = kTicosCurrentTimeType_Unknown;
    }
    s_ticos_log_data_source_ctx.num_logs = ctx.num_logs;
  }
  ticos_unlock();
}

bool ticos_log_data_source_has_been_triggered(void) {
  // Note: ticos_lock() is held when this is called by ticos_log
  return s_ticos_log_data_source_ctx.triggered;
}

typedef struct {
  size_t num_logs;
  sTicosCurrentTime trigger_time;
  sTicosCborEncoder encoder;
  bool has_encoding_error;
  bool should_stop_encoding;
  union {
    size_t num_encoded_logs;
    size_t num_marked_sent_logs;
  };
} sTcsLogEncodingCtx;

static bool prv_copy_msg_callback(sTcsLogIterator *iter, TICOS_UNUSED size_t offset,
                                  const char *buf, size_t buf_len) {
  sTcsLogEncodingCtx *const ctx = (sTcsLogEncodingCtx *)iter->user_ctx;
  return ticos_cbor_join(&ctx->encoder, buf, buf_len);
}

static bool prv_encode_current_log(sTicosCborEncoder *encoder, sTcsLogIterator *iter) {

  if (!ticos_cbor_encode_unsigned_integer(encoder,
                                             ticos_log_get_level_from_hdr(iter->entry.hdr))) {
    return false;
  }

  eTicosLogRecordType type = ticos_log_get_type_from_hdr(iter->entry.hdr);
  bool success;

  // Note: We encode "preformatted" logs (i.e logs that have run through printf) as cbor text
  // string and "compact" logs as a cbor byte array so we can differentiate between the two while
  // decoding
  if (type == kTicosLogRecordType_Preformatted) {
    success = ticos_cbor_encode_string_begin(encoder, iter->entry.len);
  } else { // kTicosLogRecordType_Compact
    success = ticos_cbor_encode_byte_string_begin(encoder, iter->entry.len);
  }

  return (success && ticos_log_iter_copy_msg(iter, prv_copy_msg_callback));
}

static bool prv_log_iterate_encode_callback(sTcsLogIterator *iter) {
  sTcsLogEncodingCtx *const ctx = (sTcsLogEncodingCtx *)iter->user_ctx;
  if (ctx->should_stop_encoding) {
    return false;
  }
  if (!prv_log_is_sent(iter->entry.hdr)) {
    ctx->has_encoding_error |= !prv_encode_current_log(&ctx->encoder, iter);
    // It's possible more logs have been added to the buffer
    // after the ticos_log_data_source_has_been_triggered() call. They cannot be included,
    // because the total message size has already been communicated to the packetizer.
    if (++ctx->num_encoded_logs == ctx->num_logs) {
      return false;
    }
  }
  return true;
}

static bool prv_encode(sTicosCborEncoder *encoder, void *iter) {
  sTcsLogEncodingCtx *ctx = (sTcsLogEncodingCtx *)((sTcsLogIterator *)iter)->user_ctx;
  if (!ticos_serializer_helper_encode_metadata_with_time(
    encoder, kTicosEventType_Logs, &ctx->trigger_time)) {
    return false;
  }
  if (!ticos_cbor_encode_unsigned_integer(encoder, kTicosEventKey_EventInfo)) {
    return false;
  }
  // To save space, all logs are encoded into a single array (as opposed to using a map or
  // array per log):
  const size_t elements_per_log = 2;  // level, msg
  if (!ticos_cbor_encode_array_begin(encoder, elements_per_log * ctx->num_logs)) {
    return false;
  }
  ticos_log_iterate(prv_log_iterate_encode_callback, iter);
  return ctx->has_encoding_error;
}

static void prv_init_encoding_ctx(sTcsLogEncodingCtx *ctx) {
  *ctx = (sTcsLogEncodingCtx) {
    .num_logs = s_ticos_log_data_source_ctx.num_logs,
    .trigger_time = s_ticos_log_data_source_ctx.trigger_time,
  };
}

static bool prv_has_logs(size_t *total_size) {
  if (!s_ticos_log_data_source_ctx.triggered) {
    return false;
  }

  sTcsLogEncodingCtx ctx;
  prv_init_encoding_ctx(&ctx);

  sTcsLogIterator iter = {
    .read_offset = 0,
    .user_ctx = &ctx
  };

  *total_size = ticos_serializer_helper_compute_size(&ctx.encoder, prv_encode, &iter);
  return true;
}

typedef struct {
  uint32_t offset;
  uint8_t *buf;
  size_t buf_len;
  size_t data_source_bytes_written;
  sTcsLogEncodingCtx encoding_ctx;
} sTcsLogsDestCtx;

static void prv_encoder_callback(void *encoder_ctx,
                                 uint32_t src_offset, const void *src_buf, size_t src_buf_len) {
  sTcsLogsDestCtx *dest = (sTcsLogsDestCtx *)encoder_ctx;

  const size_t dest_end_offset = dest->offset + dest->buf_len;
  // Optimization: stop encoding if the encoder writes are past the destination buffer:
  if (src_offset > dest_end_offset) {
    dest->encoding_ctx.should_stop_encoding = true;
    return;
  }
  const size_t src_end_offset = src_offset + src_buf_len;
  const size_t intersection_start_offset = TICOS_MAX(src_offset, dest->offset);
  const size_t intersection_end_offset = TICOS_MIN(src_end_offset, dest_end_offset);
  if (intersection_end_offset <= intersection_start_offset) {
    return;  // no intersection
  }
  const size_t intersection_len = intersection_end_offset - intersection_start_offset;
  memcpy(dest->buf + (intersection_start_offset - dest->offset),
         ((const uint8_t *)src_buf) + (intersection_start_offset - src_offset),
         intersection_len);
  dest->data_source_bytes_written += intersection_len;
}

static bool prv_logs_read(uint32_t offset, void *buf, size_t buf_len) {
  sTcsLogsDestCtx dest_ctx = (sTcsLogsDestCtx) {
    .offset = offset,
    .buf = buf,
    .buf_len = buf_len,
  };

  sTcsLogIterator iter = {
    .user_ctx = &dest_ctx.encoding_ctx,
  };

  prv_init_encoding_ctx(&dest_ctx.encoding_ctx);
  // Note: UINT_MAX is passed as length, because it is possible and expected that the output is written
  // partially by the callback. The callback takes care of not overrunning the output buffer itself.
  ticos_cbor_encoder_init(&dest_ctx.encoding_ctx.encoder, prv_encoder_callback, &dest_ctx, UINT32_MAX);
  prv_encode(&dest_ctx.encoding_ctx.encoder, &iter);
  return buf_len == dest_ctx.data_source_bytes_written;
}

static bool prv_log_iterate_mark_sent_callback(sTcsLogIterator *iter) {
  sTcsLogEncodingCtx *const ctx = (sTcsLogEncodingCtx *)iter->user_ctx;
  if (!prv_log_is_sent(iter->entry.hdr)) {
    iter->entry.hdr |= TICOS_LOG_HDR_SENT_MASK;
    ticos_log_iter_update_entry(iter);
    if (++ctx->num_marked_sent_logs == ctx->num_logs) {
      return false;
    }
  }
  return true;
}

static void prv_logs_mark_sent(void) {
  sTcsLogEncodingCtx ctx;

  sTcsLogIterator iter = {
    .read_offset = 0,
    .user_ctx = &ctx
  };

  prv_init_encoding_ctx(&ctx);
  ticos_log_iterate(prv_log_iterate_mark_sent_callback, &iter);

  ticos_lock();
  s_ticos_log_data_source_ctx = (sTcsLogDataSourceCtx) { 0 };
  ticos_unlock();
}

//! Expose a data source for use by the Ticos Packetizer
const sTicosDataSourceImpl g_ticos_log_data_source  = {
  .has_more_msgs_cb = prv_has_logs,
  .read_msg_cb = prv_logs_read,
  .mark_msg_read_cb = prv_logs_mark_sent,
};

void ticos_log_data_source_reset(void) {
  s_ticos_log_data_source_ctx = (sTcsLogDataSourceCtx) { 0 };
}

size_t ticos_log_data_source_count_unsent_logs(void) {
  sTcsLogCountingCtx ctx = { 0 };
  sTcsLogIterator iter = { .user_ctx = &ctx };
  ticos_log_iterate(prv_log_iterate_counting_callback, &iter);
  return ctx.num_logs;
}

#endif /* TICOS_LOG_DATA_SOURCE_ENABLED */
