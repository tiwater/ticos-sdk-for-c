//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

#include "ticos/util/rle.h"

#include <stdbool.h>
#include <stddef.h>

#include "ticos/core/compiler.h"
#include "ticos/util/varint.h"

TICOS_STATIC_ASSERT(sizeof(((sTicosRleWriteInfo *)0x0)->header) == TICOS_UINT32_MAX_VARINT_LENGTH,
                       "header buffer not appropriately sized");

static void prv_handle_rle_change(sTicosRleCtx *ctx) {
  // Are we currently encoding a repeat sequence?
  const bool repeated_pattern = ctx->state == kTicosRleState_RepeatSeq;

  ctx->write_info = (sTicosRleWriteInfo) {
    .available = true,
    .write_start_offset = ctx->seq_start_offset,
    .write_len = repeated_pattern ? 1 : ctx->seq_count,
  };

  int32_t rle_size = (int)(repeated_pattern ? ctx->seq_count : -ctx->seq_count);

  ctx->write_info.header_len = ticos_encode_varint_si32(rle_size, &ctx->write_info.header[0]);

  ctx->total_rle_size += ctx->write_info.header_len + ctx->write_info.write_len;

  if (repeated_pattern) {
    ctx->seq_start_offset = ctx->curr_offset;
    ctx->seq_count = 0;
  } else {
    // We've found a minimal length repeat sequence to encode
    ctx->seq_start_offset = ctx->curr_offset - ctx->num_repeats;
    ctx->seq_count = ctx->num_repeats;
  }
}

void ticos_rle_encode_finalize(sTicosRleCtx *ctx) {
  prv_handle_rle_change(ctx);
}

size_t ticos_rle_encode(sTicosRleCtx *ctx, const void *buf, size_t buf_size) {
  if (buf == NULL || buf_size == 0) {
    return 0;
  }

  // NB: The caller should check this between calls to find out if a new sequence to
  // write has been detected so we reset it upon every invocation
  ctx->write_info = (sTicosRleWriteInfo) { 0 };

  const uint32_t start_offset = ctx->curr_offset;
  const uint8_t *byte_buf = buf;
  for (uint32_t i = 0; i < buf_size; i++) {
    const uint8_t byte = byte_buf[i];

    // NB: We flag the first encoded byte as a repeat sequence until proven otherwise
    const bool is_repeat_seq = (ctx->curr_offset != 0) && ctx->last_byte == byte;
    if ((ctx->curr_offset != 0) && (ctx->last_byte == byte)) {
      ctx->num_repeats++;
    } else {
      ctx->num_repeats = 0;
    }

    switch (ctx->state) {
      case kTicosRleState_RepeatSeq:
        if (!is_repeat_seq) {
          prv_handle_rle_change(ctx);
        }

        // Starting a new sequence, flag the current byte as non-repeating until proven otherwise
        ctx->state = kTicosRleState_NonRepeatSeq;
        break;
      case kTicosRleState_NonRepeatSeq:
        // NB: If we only have two repeating bytes and leading non-repeating bytes we
        // want to encode this as one sequence to save space:
        //
        // 1, 2, 2, 3 encoded as a non repeat + repeast is 6 bytes: (-1), 1, (2), 2, (-1), 3
        // whereas
        // 1, 2, 2, 3 encoded as one sequence is 5 bytes: (-4), 1, 2, 2, 3

        if (is_repeat_seq && ctx->num_repeats >= 2) {
          ctx->seq_count -= ctx->num_repeats;
          prv_handle_rle_change(ctx);
        }
        break;
      case kTicosRleState_Init:
        ctx->state = kTicosRleState_NonRepeatSeq;
        break;
      default:
        break;
    }

    if (ctx->num_repeats >= 1 && ctx->seq_count == ctx->num_repeats) {
      // The sequence currently being encoded is comprised of at least
      // two repeating bytes so let's mark the state as repeating
      ctx->state = kTicosRleState_RepeatSeq;
    }

    ctx->last_byte = byte;
    ctx->seq_count++;
    ctx->curr_offset++;
    if (ctx->write_info.available) {
      break;
    }
  }

  return ctx->curr_offset - start_offset;
}
