#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Utilities to assist with querying the log buffer
//!
//! @note A user of the Ticos SDK should _never_ call any
//! of these routines directly

#include <stdbool.h>
#include <stdint.h>

#include "ticos/core/compiler.h"
#include "ticos/core/log.h"
#include "ticos/core/platform/debug_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Note: We do not use bitfields here to avoid portability complications on the decode side since
// alignment of bitfields as well as the order of bitfields is left up to the compiler per the C
// standard.
//
// Header Layout:
// 0brsxx.tlll
// where
//  r = read (1 if the message has been read, 0 otherwise)
//  s = sent (1 if the message has been sent, 0 otherwise)
//  x = rsvd
//  t = type (0 = formatted log, 1 = compact log)
//  l = log level (eTicosPlatformLogLevel)

#define TICOS_LOG_HDR_LEVEL_POS  0
#define TICOS_LOG_HDR_LEVEL_MASK 0x07u
#define TICOS_LOG_HDR_TYPE_POS   3u
#define TICOS_LOG_HDR_TYPE_MASK  0x08u
#define TICOS_LOG_HDR_READ_MASK  0x80u  // Log has been read through ticos_log_read()
#define TICOS_LOG_HDR_SENT_MASK  0x40u  // Log has been sent through g_ticos_log_data_source

static inline eTicosPlatformLogLevel ticos_log_get_level_from_hdr(uint8_t hdr) {
  return (eTicosPlatformLogLevel)((hdr & TICOS_LOG_HDR_LEVEL_MASK) >> TICOS_LOG_HDR_LEVEL_POS);
}

static inline eTicosLogRecordType ticos_log_get_type_from_hdr(uint8_t hdr) {
  return (eTicosLogRecordType)((hdr & TICOS_LOG_HDR_TYPE_MASK) >> TICOS_LOG_HDR_TYPE_POS);
}

typedef TICOS_PACKED_STRUCT {
  // data about the message stored (details below)
  uint8_t hdr;
  // the length of the msg
  uint8_t len;
  // underlying message
  uint8_t msg[];
} sTcsRamLogEntry;

typedef struct {
  uint32_t read_offset;
  void *user_ctx;
  sTcsRamLogEntry entry;
} sTcsLogIterator;

//! The callback invoked when "ticos_log_iterate" is called
//!
//! @param ctx The context provided to "ticos_log_iterate"
//! @param iter The iterator originally passed to "ticos_log_iterate". The
//! iter->entry field gets updated before entering this callback. The
//! iter->read_offset field gets updated after exiting this callback.
//!
//! @return bool to continue iterating, else false
typedef bool (*TicosLogIteratorCallback)(sTcsLogIterator *iter);

//! Iterates over the logs in the buffer, calling the callback for each log.
void ticos_log_iterate(TicosLogIteratorCallback callback, sTcsLogIterator *iter);

//! Update/rewrite the entry header at the position of the iterator.
//! @note This MUST ONLY be called from a ticos_log_iterate() callback (it
//! assumes ticos_lock has been taken by the caller).
bool ticos_log_iter_update_entry(sTcsLogIterator *iter);

typedef bool (* TicosLogMsgCopyCallback)(sTcsLogIterator *iter, size_t offset,
                                            const char *buf, size_t buf_len);

//! @note This MUST ONLY be called from a ticos_log_iterate() callback (it
//! assumes ticos_lock has been taken by the caller).
bool ticos_log_iter_copy_msg(sTcsLogIterator *iter, TicosLogMsgCopyCallback callback);

#ifdef __cplusplus
}
#endif
