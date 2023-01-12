//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A simple RAM backed logging storage implementation. When a device crashes and the memory region
//! is collected using the panics component, the logs will be decoded and displayed in the Ticos
//! cloud UI.

#include "ticos/core/log.h"
#include "ticos/core/log_impl.h"
#include "ticos_log_private.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/core/platform/overrides.h"
#include "ticos/util/circular_buffer.h"
#include "ticos/util/crc16_ccitt.h"
#include "ticos/core/compact_log_serializer.h"

#include "ticos/config.h"

#if TICOS_LOG_DATA_SOURCE_ENABLED
#include "ticos_log_data_source_private.h"
#endif

#define TICOS_RAM_LOGGER_VERSION 1

typedef struct TcsLogStorageInfo {
  void *storage;
  size_t len;
  uint16_t crc16;
} sTcsLogStorageRegionInfo;

typedef struct {
  uint8_t version;
  bool enabled;
  // the minimum log level level that will be saved
  // Can be changed with ticos_log_set_min_save_level()
  eTicosPlatformLogLevel min_log_level;
  uint32_t log_read_offset;
  // The number of messages that were flushed without ever being read. If ticos_log_read() is
  // not used by a platform, this will be equivalent to the number of messages logged since boot
  // that are no longer in the log buffer.
  uint32_t dropped_msg_count;
  sTcsCircularBuffer circ_buffer;
  // When initialized we keep track of the user provided storage buffer and crc the location +
  // size. When the system crashes we can check to see if this info has been corrupted in any way
  // before trying to collect the region.
  sTcsLogStorageRegionInfo region_info;
} sTcsRamLogger;

static sTcsRamLogger s_ticos_ram_logger = {
  .enabled = false,
};

static uint16_t prv_compute_log_region_crc16(void) {
  return ticos_crc16_ccitt_compute(
      TICOS_CRC16_CCITT_INITIAL_VALUE, &s_ticos_ram_logger.region_info,
      offsetof(sTcsLogStorageRegionInfo, crc16));
}

bool ticos_log_get_regions(sTicosLogRegions *regions) {
  if (!s_ticos_ram_logger.enabled) {
    return false;
  }

  const sTcsLogStorageRegionInfo *region_info = &s_ticos_ram_logger.region_info;
  const uint16_t current_crc16 = prv_compute_log_region_crc16();
  if (current_crc16 != region_info->crc16) {
    return false;
  }

  *regions = (sTicosLogRegions) {
    .region = {
      {
        .region_start = &s_ticos_ram_logger,
        .region_size = sizeof(s_ticos_ram_logger),
      },
      {
        .region_start = region_info->storage,
        .region_size = region_info->len,
      }
    }
  };
  return true;
}

static uint8_t prv_build_header(eTicosPlatformLogLevel level, eTicosLogRecordType type) {
  TICOS_STATIC_ASSERT(kTicosPlatformLogLevel_NumLevels <= 8,
                         "Number of log levels exceed max number that log module can track");
  TICOS_STATIC_ASSERT(kTicosLogRecordType_NumTypes <= 2,
                         "Number of log types exceed max number that log module can track");

  const uint8_t level_field = (level << TICOS_LOG_HDR_LEVEL_POS) & TICOS_LOG_HDR_LEVEL_MASK;
  const uint8_t type_field = (type << TICOS_LOG_HDR_TYPE_POS) & TICOS_LOG_HDR_TYPE_MASK;
  return level_field | type_field;
}

void ticos_log_set_min_save_level(eTicosPlatformLogLevel min_log_level) {
  s_ticos_ram_logger.min_log_level = min_log_level;
}

static bool prv_try_free_space(sTcsCircularBuffer *circ_bufp, int bytes_needed) {
  const size_t bytes_free = ticos_circular_buffer_get_write_size(circ_bufp);
  bytes_needed -= bytes_free;
  if (bytes_needed <= 0) {
    // no work to do, there is already enough space available
    return true;
  }

  size_t tot_read_space = ticos_circular_buffer_get_read_size(circ_bufp);
  if (bytes_needed > (int)tot_read_space) {
    // Even if we dropped all the data in the buffer there would not be enough space
    // This means the message is larger than the storage area we have allocated for the buffer
    return false;
  }

#if TICOS_LOG_DATA_SOURCE_ENABLED
  if (ticos_log_data_source_has_been_triggered()) {
    // Don't allow expiring old logs. ticos_log_trigger_collection() has been
    // called, so we're in the process of uploading the logs in the buffer.
    return false;
  }
#endif

  // Expire oldest logs until there is enough room available
  while (tot_read_space != 0) {
    sTcsRamLogEntry curr_entry = { 0 };
    ticos_circular_buffer_read(circ_bufp, 0, &curr_entry, sizeof(curr_entry));
    const size_t space_to_free = curr_entry.len + sizeof(curr_entry);

    if ((curr_entry.hdr & TICOS_LOG_HDR_READ_MASK) != 0) {
      s_ticos_ram_logger.log_read_offset -= space_to_free;
    } else {
      // We are removing a message that was not read via ticos_log_read().
      s_ticos_ram_logger.dropped_msg_count++;
    }

    ticos_circular_buffer_consume(circ_bufp, space_to_free);
    bytes_needed -= space_to_free;
    if (bytes_needed <= 0) {
      return true;
    }
    tot_read_space = ticos_circular_buffer_get_read_size(circ_bufp);
  }

  return false; // should be unreachable
}

static void prv_iterate(TicosLogIteratorCallback callback, sTcsLogIterator *iter) {
  sTcsCircularBuffer *const circ_bufp = &s_ticos_ram_logger.circ_buffer;
  bool should_continue = true;
  while (should_continue) {
    if (!ticos_circular_buffer_read(
        circ_bufp, iter->read_offset, &iter->entry, sizeof(iter->entry))) {
      return;
    }

    // Note: At this point, the ticos_log_iter_update_entry(),
    // ticos_log_entry_get_msg_pointer() calls made from the callback should never fail.
    // A failure is indicative of memory corruption (e.g calls taking place from multiple tasks
    // without having implemented ticos_lock() / ticos_unlock())

    should_continue = callback(iter);
    iter->read_offset += sizeof(iter->entry) + iter->entry.len;
  }
}

void ticos_log_iterate(TicosLogIteratorCallback callback, sTcsLogIterator *iter) {
  ticos_lock();
  prv_iterate(callback, iter);
  ticos_unlock();
}

bool ticos_log_iter_update_entry(sTcsLogIterator *iter) {
  sTcsCircularBuffer *const circ_bufp = &s_ticos_ram_logger.circ_buffer;
  const size_t offset_from_end =
      ticos_circular_buffer_get_read_size(circ_bufp) - iter->read_offset;
  return ticos_circular_buffer_write_at_offset(
      circ_bufp, offset_from_end, &iter->entry, sizeof(iter->entry));
}

bool ticos_log_iter_copy_msg(sTcsLogIterator *iter, TicosLogMsgCopyCallback callback) {
  sTcsCircularBuffer *const circ_bufp = &s_ticos_ram_logger.circ_buffer;
  return ticos_circular_buffer_read_with_callback(
    circ_bufp, iter->read_offset + sizeof(iter->entry), iter->entry.len, iter,
    (TicosCircularBufferReadCallback)callback);
}

typedef struct {
  sTicosLog *log;
  bool has_log;
} sTcsReadLogCtx;

static bool prv_read_log_iter_callback(sTcsLogIterator *iter) {
  sTcsReadLogCtx *const ctx = (sTcsReadLogCtx *)iter->user_ctx;
  sTcsCircularBuffer *const circ_bufp = &s_ticos_ram_logger.circ_buffer;

  // mark the message as read
  iter->entry.hdr |= TICOS_LOG_HDR_READ_MASK;
  if (!ticos_log_iter_update_entry(iter)) {
    return false;
  }

  if (!ticos_circular_buffer_read(
      circ_bufp, iter->read_offset + sizeof(iter->entry), ctx->log->msg, iter->entry.len)) {
    return false;
  }

  ctx->log->msg[iter->entry.len] = '\0';
  ctx->log->level = ticos_log_get_level_from_hdr(iter->entry.hdr);
  ctx->log->type = ticos_log_get_type_from_hdr(iter->entry.hdr);
  ctx->log->msg_len = iter->entry.len;
  ctx->has_log = true;
  return false;
}

static bool prv_read_log(sTicosLog *log) {
  if (s_ticos_ram_logger.dropped_msg_count) {
    log->level = kTicosPlatformLogLevel_Warning;
    const int rv = snprintf(log->msg, sizeof(log->msg), "... %d messages dropped ...",
                                 (int)s_ticos_ram_logger.dropped_msg_count);
    log->msg_len = (rv <= 0)  ? 0 : TICOS_MIN((uint32_t)rv, sizeof(log->msg) - 1);
    log->type = kTicosLogRecordType_Preformatted;
    s_ticos_ram_logger.dropped_msg_count = 0;
    return true;
  }

  sTcsReadLogCtx user_ctx = {
    .log = log
  };

  sTcsLogIterator iter = {
    .read_offset = s_ticos_ram_logger.log_read_offset,

    .user_ctx = &user_ctx
  };

  prv_iterate(prv_read_log_iter_callback, &iter);
  s_ticos_ram_logger.log_read_offset = iter.read_offset;
  return user_ctx.has_log;
}

bool ticos_log_read(sTicosLog *log) {
  if (!s_ticos_ram_logger.enabled || (log == NULL)) {
    return false;
  }

  ticos_lock();
  const bool found_unread_log = prv_read_log(log);
  ticos_unlock();

  return found_unread_log;
}

static bool prv_should_log(eTicosPlatformLogLevel level) {
  if (!s_ticos_ram_logger.enabled) {
    return false;
  }

  if (level < s_ticos_ram_logger.min_log_level) {
    return false;
  }

  return true;
}

//! Stub implementation that a user of the SDK can override. See header for more details.
TICOS_WEAK void ticos_log_handle_saved_callback(void) {
  return;
}

void ticos_vlog_save(eTicosPlatformLogLevel level, const char *fmt, va_list args) {
  if (!prv_should_log(level)) {
    return;
  }

  char log_buf[TICOS_LOG_MAX_LINE_SAVE_LEN + 1];

  const size_t available_space = sizeof(log_buf);
  const int rv = vsnprintf(log_buf, available_space, fmt, args);

  if (rv <= 0) {
    return;
  }

  size_t bytes_written = (size_t)rv;
  if (bytes_written >= available_space) {
    bytes_written = available_space - 1;
  }

  ticos_log_save_preformatted(level, log_buf, bytes_written);
}

void ticos_log_save(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  ticos_vlog_save(level, fmt, args);
  va_end(args);
}

static void prv_log_save(eTicosPlatformLogLevel level,
                         const void *log, size_t log_len,
                         eTicosLogRecordType log_type) {

  if (!prv_should_log(level)) {
    return;
  }

  bool log_written = false;
  const size_t truncated_log_len = TICOS_MIN(log_len, TICOS_LOG_MAX_LINE_SAVE_LEN);
  const size_t bytes_needed = sizeof(sTcsRamLogEntry) + truncated_log_len;
  ticos_lock();
  {
    sTcsCircularBuffer *circ_bufp = &s_ticos_ram_logger.circ_buffer;
    const bool space_free = prv_try_free_space(circ_bufp, (int)bytes_needed);
    if (space_free) {
        sTcsRamLogEntry entry = {
          .len = (uint8_t)truncated_log_len,
          .hdr = prv_build_header(level, log_type),
        };
        ticos_circular_buffer_write(circ_bufp, &entry, sizeof(entry));
        ticos_circular_buffer_write(circ_bufp, log, truncated_log_len);
        log_written = true;
    }
  }
  ticos_unlock();

  if (log_written) {
    ticos_log_handle_saved_callback();
  }
}

#if TICOS_COMPACT_LOG_ENABLE

static void prv_fill_compact_log_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  uint8_t *log = (uint8_t *)ctx;
  memcpy(&log[offset], buf, buf_len);
}

void ticos_compact_log_save(eTicosPlatformLogLevel level, uint32_t log_id,
                               uint32_t compressed_fmt, ...) {
  char log_buf[TICOS_LOG_MAX_LINE_SAVE_LEN + 1];

  sTicosCborEncoder encoder;
  ticos_cbor_encoder_init(&encoder, prv_fill_compact_log_cb, log_buf, sizeof(log_buf));

  va_list args;
  va_start(args, compressed_fmt);
  bool success = ticos_vlog_compact_serialize(&encoder, log_id, compressed_fmt, args);
  va_end(args);

  if (!success) {
    return;
  }

  const size_t bytes_written = ticos_cbor_encoder_deinit(&encoder);
  prv_log_save(level, log_buf, bytes_written, kTicosLogRecordType_Compact);
}

#endif /* TICOS_COMPACT_LOG_ENABLE */


void ticos_log_save_preformatted(eTicosPlatformLogLevel level,
                                    const char *log, size_t log_len) {
  prv_log_save(level, log, log_len, kTicosLogRecordType_Preformatted);
}

bool ticos_log_boot(void *storage_buffer, size_t buffer_len) {
  if (storage_buffer == NULL || buffer_len == 0 || s_ticos_ram_logger.enabled) {
    return false;
  }

  s_ticos_ram_logger = (sTcsRamLogger) {
    .version = TICOS_RAM_LOGGER_VERSION,
    .min_log_level = TICOS_RAM_LOGGER_DEFAULT_MIN_LOG_LEVEL,
    .region_info = {
      .storage = storage_buffer,
      .len = buffer_len,
    },
  };

  s_ticos_ram_logger.region_info.crc16 = prv_compute_log_region_crc16();

  ticos_circular_buffer_init(&s_ticos_ram_logger.circ_buffer, storage_buffer, buffer_len);

  // finally, enable logging
  s_ticos_ram_logger.enabled = true;
  return true;
}

void ticos_log_reset(void) {
  s_ticos_ram_logger = (sTcsRamLogger) {
    .enabled = false,
  };
}
