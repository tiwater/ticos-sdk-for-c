//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/core/data_packetizer.h"

#include <string.h>
#include <inttypes.h>

#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/data_source_rle.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/util/chunk_transport.h"

TICOS_STATIC_ASSERT(TICOS_PACKETIZER_MIN_BUF_LEN == TICOS_MIN_CHUNK_BUF_LEN,
                       "Minimum packetizer payload size must match underlying transport");
//
// Weak definitions which get overridden when the component that implements that data source is
// included and compiled in a project
//

static bool prv_data_source_has_event_stub(size_t *event_size) {
  *event_size = 0;
  return false;
}

static bool prv_data_source_read_stub(TICOS_UNUSED uint32_t offset,
                                      TICOS_UNUSED void *buf,
                                      TICOS_UNUSED size_t buf_len) {
  return false;
}

static void prv_data_source_mark_event_read_stub(void) { }

TICOS_WEAK const sTicosDataSourceImpl g_ticos_data_rle_source = {
  .has_more_msgs_cb = prv_data_source_has_event_stub,
  .read_msg_cb = prv_data_source_read_stub,
  .mark_msg_read_cb = prv_data_source_mark_event_read_stub,
};

TICOS_WEAK const sTicosDataSourceImpl g_ticos_coredump_data_source = {
  .has_more_msgs_cb = prv_data_source_has_event_stub,
  .read_msg_cb = prv_data_source_read_stub,
  .mark_msg_read_cb = prv_data_source_mark_event_read_stub,
};

TICOS_WEAK const sTicosDataSourceImpl g_ticos_event_data_source = {
  .has_more_msgs_cb = prv_data_source_has_event_stub,
  .read_msg_cb = prv_data_source_read_stub,
  .mark_msg_read_cb = prv_data_source_mark_event_read_stub,
};

TICOS_WEAK const sTicosDataSourceImpl g_ticos_log_data_source = {
  .has_more_msgs_cb = prv_data_source_has_event_stub,
  .read_msg_cb = prv_data_source_read_stub,
  .mark_msg_read_cb = prv_data_source_mark_event_read_stub,
};

TICOS_WEAK const sTicosDataSourceImpl g_ticos_cdr_source = {
  .has_more_msgs_cb = prv_data_source_has_event_stub,
  .read_msg_cb = prv_data_source_read_stub,
  .mark_msg_read_cb = prv_data_source_mark_event_read_stub,
};

TICOS_WEAK
bool ticos_data_source_rle_encoder_set_active(
    TICOS_UNUSED const sTicosDataSourceImpl *active_source) {
  return false;
}

// NOTE: These values are used by the Ticos cloud chunks API
typedef enum {
  kTcsMessageType_None = 0,
  kTcsMessageType_Coredump = 1,
  kTcsMessageType_Event = 2,
  kTcsMessageType_Log = 3,
  kTcsMessageType_Cdr = 4,
  kTcsMessageType_NumTypes
} eTcsMessageType;

//! Make sure our externally facing types match the internal ones
TICOS_STATIC_ASSERT((1 << kTcsMessageType_Coredump) == kTcsDataSourceMask_Coredump,
                       "kTcsDataSourceMask_Coredump is incorrectly defined");
TICOS_STATIC_ASSERT((1 << kTcsMessageType_Event) == kTcsDataSourceMask_Event,
                       "kTcsDataSourceMask_Event, is incorrectly defined");
TICOS_STATIC_ASSERT((1 << kTcsMessageType_Log) == kTcsDataSourceMask_Log,
                       "kTcsDataSourceMask_Log is incorrectly defined");
TICOS_STATIC_ASSERT((1 << kTcsMessageType_Cdr) == kTcsDataSourceMask_Cdr,
                       "kTcsDataSourceMask_Cdr is incorrectly defined");
TICOS_STATIC_ASSERT(kTcsMessageType_NumTypes == 5, "eTcsMessageType needs to be updated");


typedef struct TicosDataSource {
  eTcsMessageType type;
  bool use_rle;
  const sTicosDataSourceImpl *impl;
} sTicosDataSource;

static const sTicosDataSource s_ticos_data_source[] = {
  {
    .type = kTcsMessageType_Coredump,
    .use_rle = false,
    .impl = &g_ticos_coredump_data_source,
  },
  {
    .type = kTcsMessageType_Event,
    .use_rle = false,
    .impl = &g_ticos_event_data_source,
  },
  {
    .type = kTcsMessageType_Log,
    .use_rle = false,
    .impl = &g_ticos_log_data_source,
  },
  // NB: We may want to enable RLE in the future here (probably a lot of repeat patterns). The one
  // thing to keep in mind is that when the encoder is enabled, it requires a lot more short reads
  // to take place on the data source which can be a slow operation for flash based filesystems.
  {
    .type =  kTcsMessageType_Cdr,
    .use_rle = false,
    .impl = &g_ticos_cdr_source,
  }
};

typedef struct {
  size_t total_size;
  sTicosDataSource source;
} sMessageMetadata;

typedef struct {
  bool active_message;
  sMessageMetadata msg_metadata;
  sTcsChunkTransportCtx curr_msg_ctx;
} sTcsTransportState;

typedef TICOS_PACKED_STRUCT {
  uint8_t tcs_msg_type; // eTcsMessageType
} sTcsPacketizerHdr;

static sTcsTransportState s_tcs_packetizer_state;

static uint32_t s_active_data_sources = kTcsDataSourceMask_All;

void ticos_packetizer_set_active_sources(uint32_t mask) {
  ticos_packetizer_abort();
  s_active_data_sources = mask;
}

static void prv_reset_packetizer_state(void) {
  s_tcs_packetizer_state = (sTcsTransportState) {
    .active_message = false,
  };

  ticos_data_source_rle_encoder_set_active(NULL);
}

static void prv_data_source_chunk_transport_msg_reader(uint32_t offset, void *buf,
                                                       size_t buf_len) {
  uint8_t *bufp = buf;
  size_t read_offset = 0;
  const size_t hdr_size = sizeof(sTcsPacketizerHdr);

  const sMessageMetadata *msg_metadata = &s_tcs_packetizer_state.msg_metadata;
  if (offset < hdr_size) {
    const uint8_t rle_enable_mask = 0x80;
    const uint8_t msg_type = (uint8_t)msg_metadata->source.type;

    sTcsPacketizerHdr hdr = {
      .tcs_msg_type = msg_metadata->source.use_rle ? msg_type | rle_enable_mask : msg_type,
    };
    uint8_t *hdr_bytes = (uint8_t *)&hdr;

    const size_t bytes_to_copy = TICOS_MIN(hdr_size - offset, buf_len);
    memcpy(bufp, &hdr_bytes[offset], bytes_to_copy);
    bufp += bytes_to_copy;
    buf_len -= bytes_to_copy;
  } else {
    read_offset = offset - hdr_size;
  }

  if (buf_len == 0) {
    // no space left after writing the header
    return;
  }

  const bool success = msg_metadata->source.impl->read_msg_cb(read_offset, bufp, buf_len);
  if (!success) {
    // Read failures really should never happen. We have no way of knowing if the issue is
    // transient or not. If we aborted the transaction and the failure was persistent, we could get
    // stuck trying to flush out the same data. Instead, we just continue on. We scrub the
    // beginning of the chunk buffer with a known pattern to make the error easier to identify.
    TICOS_LOG_ERROR("Read at offset 0x%" PRIx32 " (%d bytes) for source type %d failed", offset,
                       (int)buf_len, (int)msg_metadata->source.type);
    memset(bufp, 0xEF, TICOS_MIN(16, buf_len));
  }
}

static bool prv_get_source_with_data(size_t *total_size, sTicosDataSource *active_source) {
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_ticos_data_source); i++) {
    const sTicosDataSource *data_source = &s_ticos_data_source[i];

    const bool disabled_source = (((1 << data_source->type) & s_active_data_sources) == 0);
    if (disabled_source) {
      // sdk user has disabled extraction of data for specified source
      continue;
    }

    const bool rle_enabled = data_source->use_rle &&
        ticos_data_source_rle_encoder_set_active(data_source->impl);

    *active_source = (sTicosDataSource) {
      .type = data_source->type,
      .use_rle = rle_enabled,
      .impl = rle_enabled ? &g_ticos_data_rle_source : data_source->impl,
    };

    if (active_source->impl->has_more_msgs_cb(total_size)) {
      return true;
    }
  }
  return false;
}

static bool prv_more_messages_to_send(sMessageMetadata *msg_metadata) {
  size_t total_size;
  sTicosDataSource active_source;
  if (!prv_get_source_with_data(&total_size, &active_source)) {
    return false;
  }

  if (msg_metadata != NULL) {
    *msg_metadata = (sMessageMetadata) {
      .total_size = total_size,
      .source = active_source,
    };
  }

  return true;
}

static bool prv_load_next_message_to_send(bool enable_multi_packet_chunks,
                                          sTcsTransportState *state) {
  sMessageMetadata msg_metadata;
  if (!prv_more_messages_to_send(&msg_metadata)) {
    return false;
  }

  *state = (sTcsTransportState) {
    .active_message = true,
    .msg_metadata = msg_metadata,
    .curr_msg_ctx = (sTcsChunkTransportCtx) {
      .total_size = msg_metadata.total_size + sizeof(sTcsPacketizerHdr),
      .read_msg = prv_data_source_chunk_transport_msg_reader,
      .enable_multi_call_chunk = enable_multi_packet_chunks,
    },
  };
  ticos_chunk_transport_get_chunk_info(&s_tcs_packetizer_state.curr_msg_ctx);
  return true;
}

static void prv_mark_message_send_complete_and_cleanup(void) {
  // we've finished sending the data so delete it
  s_tcs_packetizer_state.msg_metadata.source.impl->mark_msg_read_cb();

  prv_reset_packetizer_state();
}

void ticos_packetizer_abort(void) {
  prv_reset_packetizer_state();
}

eTicosPacketizerStatus ticos_packetizer_get_next(void *buf, size_t *buf_len) {
  if (buf == NULL || buf_len == NULL) {
    // We may want to consider just asserting on these. For now, just log an error
    // and return NoMoreData
    TICOS_LOG_ERROR("%s: NULL input arguments", __func__);
    return kTicosPacketizerStatus_NoMoreData;
  }

  if (!s_tcs_packetizer_state.active_message) {
    // To load a new message, ticos_packetizer_begin() must first be called
    return kTicosPacketizerStatus_NoMoreData;
  }

  size_t original_size = *buf_len;
  bool md = ticos_chunk_transport_get_next_chunk(
      &s_tcs_packetizer_state.curr_msg_ctx, buf, buf_len);

  if (*buf_len == 0) {
    TICOS_LOG_ERROR("Buffer of %d bytes too small to packetize data",
                       (int)original_size);
  }

  if (!md) {
    // the entire message has been chunked up, perform clean up
    prv_mark_message_send_complete_and_cleanup();

    // we have reached the end of a message
    return kTicosPacketizerStatus_EndOfChunk;
  }

  return s_tcs_packetizer_state.curr_msg_ctx.enable_multi_call_chunk ?
      kTicosPacketizerStatus_MoreDataForChunk : kTicosPacketizerStatus_EndOfChunk;
}

bool ticos_packetizer_begin(const sPacketizerConfig *cfg,
                               sPacketizerMetadata *metadata_out) {
  if ((cfg == NULL) || (metadata_out == NULL)) {
    TICOS_LOG_ERROR("%s: NULL input arguments", __func__);
    return false;
  }

  if (!s_tcs_packetizer_state.active_message) {
    if (!prv_load_next_message_to_send(cfg->enable_multi_packet_chunk, &s_tcs_packetizer_state)) {
      // no new messages to send
      *metadata_out = (sPacketizerMetadata) { 0 };
      return false;
    }
  }

  const bool send_in_progress = s_tcs_packetizer_state.curr_msg_ctx.read_offset != 0;
  *metadata_out = (sPacketizerMetadata) {
    .single_chunk_message_length = s_tcs_packetizer_state.curr_msg_ctx.single_chunk_message_length,
    .send_in_progress = send_in_progress,
  };
  return true;
}

bool ticos_packetizer_data_available(void) {
  if (s_tcs_packetizer_state.active_message) {
    return true;
  }

  return prv_more_messages_to_send(NULL);
}

bool ticos_packetizer_get_chunk(void *buf, size_t *buf_len) {
  const sPacketizerConfig cfg = {
    // By setting this to false, every call to "ticos_packetizer_get_next()" will return one
    // "chunk" that you must send from the device
    .enable_multi_packet_chunk = false,
  };

  sPacketizerMetadata metadata;
  bool data_available = ticos_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    // there are no more chunks to send
    return false;
  }

  eTicosPacketizerStatus packetizer_status = ticos_packetizer_get_next(buf, buf_len);

  // We know data is available from the ticos_packetizer_begin() call above
  // so anything but kTicosPacketizerStatus_EndOfChunk is unexpected
  if (packetizer_status != kTicosPacketizerStatus_EndOfChunk) {
    TICOS_LOG_ERROR("Unexpected packetizer status: %d", (int)packetizer_status);
    return false;
  }

  return true;
}
