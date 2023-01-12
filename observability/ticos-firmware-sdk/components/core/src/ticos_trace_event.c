//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/arch.h"
#include "ticos/core/compact_log_serializer.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/event_storage_implementation.h"
#include "ticos/core/math.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/core/trace_event.h"
#include "ticos_trace_event_private.h"

#define TICOS_TRACE_EVENT_STORAGE_UNINITIALIZED (-1)
#define TICOS_TRACE_EVENT_STORAGE_OUT_OF_SPACE (-2)
#define TICOS_TRACE_EVENT_STORAGE_TOO_SMALL (-3)
#define TICOS_TRACE_EVENT_BAD_PARAM (-4)

#define TRACE_EVENT_OPT_FIELD_STATUS_MASK (1 << 0)
#define TRACE_EVENT_OPT_FIELD_LOG_MASK (1 << 1)

typedef struct {
  eTcsTraceReasonUser reason;
  void *pc_addr;
  void *return_addr;
  //! A bitmask which tracks the optional fields have been captured.
  uint32_t opt_fields;

  //
  // Optional fields which can be captured
  //

  //! A status / error code to record alongside a trace event
  int32_t status_code;
  //! A null terminated log captured alongside a trace event
  const void *log;
  size_t log_len;
} sTicosTraceEventInfo;

static struct {
  const sTicosEventStorageImpl *storage_impl;
} s_ticos_trace_event_ctx;

int ticos_trace_event_boot(const sTicosEventStorageImpl *storage_impl) {
  if (storage_impl == NULL) {
    return TICOS_TRACE_EVENT_BAD_PARAM;
  }

  if (!ticos_serializer_helper_check_storage_size(
      storage_impl, ticos_trace_event_compute_worst_case_storage_size, "trace")) {
    return TICOS_TRACE_EVENT_STORAGE_TOO_SMALL;
  }

  s_ticos_trace_event_ctx.storage_impl = storage_impl;
  return 0;
}

static bool prv_encode_cb(sTicosCborEncoder *encoder, void *ctx) {
  const sTicosTraceEventInfo *info = (const sTicosTraceEventInfo *)ctx;
  uint32_t extra_event_info_pairs = 0;
  const bool status_present = (info->opt_fields & TRACE_EVENT_OPT_FIELD_STATUS_MASK) != 0;
  if (status_present) {
    extra_event_info_pairs++;
  }

  const bool log_present = (info->opt_fields & TRACE_EVENT_OPT_FIELD_LOG_MASK) != 0;
  if (log_present) {
    extra_event_info_pairs++;
  }

  sTicosTraceEventHelperInfo helper_info = {
      .reason_key = kTicosTraceInfoEventKey_UserReason,
      .reason_value = info->reason,
      .pc = (uint32_t)(uintptr_t)info->pc_addr,
      .lr = (uint32_t)(uintptr_t)info->return_addr,
      .extra_event_info_pairs = extra_event_info_pairs,
  };

  bool success = ticos_serializer_helper_encode_trace_event(encoder, &helper_info);

  if (success && status_present) {
    success = ticos_serializer_helper_encode_int32_kv_pair(encoder,
        kTicosTraceInfoEventKey_StatusCode, info->status_code);
  }

  if (success && log_present) {
#if !TICOS_COMPACT_LOG_ENABLE
    success = ticos_serializer_helper_encode_byte_string_kv_pair(encoder,
        kTicosTraceInfoEventKey_Log, info->log, info->log_len);
#else
    success = ticos_cbor_encode_unsigned_integer(encoder, kTicosTraceInfoEventKey_CompactLog) &&
        ticos_cbor_join(encoder, info->log, info->log_len);
#endif
  }

  return success;
}

typedef struct {
  uint32_t reason;
  sTicosTraceEventInfo info;
#if TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  char log[TICOS_TRACE_EVENT_MAX_LOG_LEN];
#endif
} sTicosIsrTraceEvent;

static sTicosIsrTraceEvent s_isr_trace_event;

// To keep the number of cycles spent logging a trace from an ISR to a minimum we just copy the
// values into a storage area and then flush the data after the system has returned from an ISR
static int prv_trace_event_capture_from_isr(sTicosTraceEventInfo *trace_info) {
  uint32_t expected_reason = TICOS_TRACE_REASON(Unknown);
  const uint32_t desired_reason = (uint32_t)trace_info->reason;

  if (s_isr_trace_event.reason != expected_reason) {
    return TICOS_TRACE_EVENT_STORAGE_OUT_OF_SPACE;
  }

  // NOTE: It's perfectly fine to be interrupted by a higher priority interrupt at this point.
  // In the unlikely scenario where that exception also logged a trace event we will just wind
  // up overwriting it. The actual update of the reason (32 bit write) is an atomic op
  s_isr_trace_event.reason = desired_reason;

  s_isr_trace_event.info = *trace_info;

  if (s_isr_trace_event.info.log != NULL) {
#if TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
    memcpy(s_isr_trace_event.log, trace_info->log, trace_info->log_len);
    s_isr_trace_event.info.log = &s_isr_trace_event.log[0];
#endif
  }

  return 0;
}

static int prv_trace_event_capture(sTicosTraceEventInfo *info) {
  sTicosCborEncoder encoder = { 0 };
  const bool success = ticos_serializer_helper_encode_to_storage(
      &encoder, s_ticos_trace_event_ctx.storage_impl, prv_encode_cb, info);

  if (!success) {
    return TICOS_TRACE_EVENT_STORAGE_OUT_OF_SPACE;
  }

  return 0;
}

int ticos_trace_event_try_flush_isr_event(void) {
  if (s_isr_trace_event.reason == TICOS_TRACE_REASON(Unknown)) {
    return 0;
  }

  const int rv = prv_trace_event_capture(&s_isr_trace_event.info);
  if (rv == 0) {
    // we successfully flushed the ISR event, mark the space as free to use again
    s_isr_trace_event.reason = TICOS_TRACE_REASON(Unknown);
  }
  return rv;
}

static int prv_capture_trace_event_info(sTicosTraceEventInfo *info) {
  if (s_ticos_trace_event_ctx.storage_impl == NULL) {
    return TICOS_TRACE_EVENT_STORAGE_UNINITIALIZED;
  }

  if (ticos_arch_is_inside_isr()) {
    return prv_trace_event_capture_from_isr(info);
  }

  // NOTE: We flush any ISR pended events here so that the order in which events are captured is
  // preserved. An user of the trace event API can also flush ISR events at anytime by explicitly
  // calling ticos_trace_event_try_flush_isr_event()
  const int rv = ticos_trace_event_try_flush_isr_event();
  if (rv != 0) {
    return rv;
  }

  return prv_trace_event_capture(info);
}

int ticos_trace_event_capture(eTcsTraceReasonUser reason, void *pc_addr,
                                 void *lr_addr) {
  sTicosTraceEventInfo event_info = {
    .reason = reason,
    .pc_addr = pc_addr,
    .return_addr = lr_addr,
  };
  return prv_capture_trace_event_info(&event_info);
}

int ticos_trace_event_with_status_capture(eTcsTraceReasonUser reason, void *pc_addr,
                                             void *lr_addr, int32_t status) {
  sTicosTraceEventInfo event_info = {
    .reason = reason,
    .pc_addr = pc_addr,
    .return_addr = lr_addr,
    .opt_fields = TRACE_EVENT_OPT_FIELD_STATUS_MASK,
    .status_code = status,
  };
  return prv_capture_trace_event_info(&event_info);
}

#if !TICOS_COMPACT_LOG_ENABLE

int ticos_trace_event_with_log_capture(
    eTcsTraceReasonUser reason, void *pc_addr, void *lr_addr, const char *fmt, ...) {

#if !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  // If a log capture takes place while in an ISR we just record a normal trace event
  if (ticos_arch_is_inside_isr()) {
    return ticos_trace_event_capture(reason, pc_addr, lr_addr);
  }
#endif /* !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED */

  // Note: By performing the vsnprintf in this function (rather than forward vargs in event_info),
  // the stdlib dependency only gets pulled in when using trace event logs and not for all trace
  // event types
  char log[TICOS_TRACE_EVENT_MAX_LOG_LEN] = { 0 };
  va_list args;
  va_start(args, fmt);
  int rv = vsnprintf(log, sizeof(log), fmt, args);
  size_t log_len = rv < 0 ? 0 : TICOS_MIN(sizeof(log) - 1, (size_t)rv);
  va_end(args);

  sTicosTraceEventInfo event_info = {
    .reason = reason,
    .pc_addr = pc_addr,
    .return_addr = lr_addr,
    .opt_fields = TRACE_EVENT_OPT_FIELD_LOG_MASK,
    .log = &log[0],
    .log_len = log_len,
  };
  return prv_capture_trace_event_info(&event_info);
}

#else

static void prv_fill_compact_log_cb(void *ctx, uint32_t offset, const void *buf, size_t buf_len) {
  uint8_t *log = (uint8_t *)ctx;
  memcpy(&log[offset], buf, buf_len);
}

int ticos_trace_event_with_compact_log_capture(
    eTcsTraceReasonUser reason, void *lr_addr,
    uint32_t log_id, uint32_t compressed_fmt, ...) {

#if !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  // If a log capture takes place while in an ISR we just record a normal trace event
  if (ticos_arch_is_inside_isr()) {
    return ticos_trace_event_capture(reason, 0, lr_addr);
  }
#endif /* !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED */

  va_list args;
  va_start(args, compressed_fmt);
  uint8_t log[TICOS_TRACE_EVENT_MAX_LOG_LEN] = { 0 };

  sTicosCborEncoder encoder;
  ticos_cbor_encoder_init(&encoder, prv_fill_compact_log_cb, log, sizeof(log));
  ticos_vlog_compact_serialize(&encoder, log_id, compressed_fmt, args);
  size_t log_len = ticos_cbor_encoder_deinit(&encoder);

  sTicosTraceEventInfo event_info = {
    .reason = reason,
    // Note: pc is recovered from file/line encoded in compact log so no need to collect!
    .pc_addr = 0,
    .return_addr = lr_addr,
    .opt_fields = TRACE_EVENT_OPT_FIELD_LOG_MASK,
    .log = &log[0],
    .log_len = log_len,
  };
  va_end(args);

  return prv_capture_trace_event_info(&event_info);
}

#endif

size_t ticos_trace_event_compute_worst_case_storage_size(void) {
  sTicosTraceEventInfo event_info = {
    .reason =  kTcsTraceReasonUser_NumReasons,
    .pc_addr = (void *)(uintptr_t)UINT32_MAX,
    .return_addr = (void *)(uintptr_t)UINT32_MAX,
    .opt_fields = TRACE_EVENT_OPT_FIELD_STATUS_MASK,
    .status_code = INT32_MAX,
  };
  sTicosCborEncoder encoder = { 0 };
  return ticos_serializer_helper_compute_size(&encoder, prv_encode_cb, &event_info);
}

void ticos_trace_event_reset(void) {
  s_ticos_trace_event_ctx.storage_impl = NULL;
  s_isr_trace_event = (sTicosIsrTraceEvent) { 0 };
}
