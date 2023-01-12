//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Reads the current reboot tracking information and converts it into an "trace" event which can
//! be sent to the Ticos cloud

#include "ticos_reboot_tracking_private.h"

#include <string.h>

#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/event_storage_implementation.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/core/serializer_key_ids.h"
#include "ticos/util/cbor.h"

#define TICOS_REBOOT_TRACKING_BAD_PARAM (-1)
#define TICOS_REBOOT_TRACKING_STORAGE_TOO_SMALL (-2)

static bool prv_serialize_reboot_info(sTicosCborEncoder *e,
                                      const sTcsResetReasonInfo *info) {
  const size_t extra_event_info_pairs = 1 /* coredump_saved */ +
                                        ((info->reset_reason_reg0 != 0) ? 1 : 0);
  const sTicosTraceEventHelperInfo helper_info = {
      .reason_key = kTicosTraceInfoEventKey_Reason,
      .reason_value = info->reason,
      .pc = info->pc,
      .lr = info->lr,
      .extra_event_info_pairs = extra_event_info_pairs,
  };
  bool success = ticos_serializer_helper_encode_trace_event(e, &helper_info);

  if (success && info->reset_reason_reg0) {
    success = ticos_serializer_helper_encode_uint32_kv_pair(e,
        kTicosTraceInfoEventKey_McuReasonRegister, info->reset_reason_reg0);
  }

  if (success) {
    success = ticos_serializer_helper_encode_uint32_kv_pair(
        e, kTicosTraceInfoEventKey_CoredumpSaved, info->coredump_saved);
  }

  return success;
}

static bool prv_encode_cb(sTicosCborEncoder *encoder, void *ctx) {
  const sTcsResetReasonInfo *info = (const sTcsResetReasonInfo *)ctx;
  return prv_serialize_reboot_info(encoder, info);
}

size_t ticos_reboot_tracking_compute_worst_case_storage_size(void) {
  // a reset reason with maximal values so we can compute the worst case encoding size
  sTcsResetReasonInfo reset_reason = {
    .reason = kTcsRebootReason_HardFault,
    .pc = UINT32_MAX,
    .lr = UINT32_MAX,
    .reset_reason_reg0 = UINT32_MAX,
    .coredump_saved = 1,
  };

  sTicosCborEncoder encoder = { 0 };
  return ticos_serializer_helper_compute_size(&encoder, prv_encode_cb, &reset_reason);
}

int ticos_reboot_tracking_collect_reset_info(const sTicosEventStorageImpl *impl) {
  if (impl == NULL) {
    return TICOS_REBOOT_TRACKING_BAD_PARAM;
  }

  ticos_serializer_helper_check_storage_size(
      impl, ticos_reboot_tracking_compute_worst_case_storage_size, "reboot");
  // we'll fall through and try to encode anyway and later return a failure
  // code if the event could not be stored. This line is here to give the user
  // an idea of how they should size things

  sTcsResetReasonInfo info;
  if (!ticos_reboot_tracking_read_reset_info(&info)) {
    // Two ways we get here:
    //  1. ticos_reboot_tracking_boot() has not yet been called
    //  2. ticos_reboot_tracking_boot() was called but there's no info
    //     about the last reboot reason. To fix this, pass bootup_info when
    //     calling ticos_reboot_tracking_boot()
    // For more details about reboot tracking in general see https://ticos.io//2QlOlgH
    TICOS_LOG_WARN("%s: No reset info collected", __func__);
    return 0;
  }

  sTicosCborEncoder encoder = { 0 };
  const bool success = ticos_serializer_helper_encode_to_storage(
      &encoder, impl, prv_encode_cb, &info);

  if (!success) {
    const size_t storage_max_size = impl->get_storage_size_cb();
    const size_t worst_case_size_needed =
        ticos_reboot_tracking_compute_worst_case_storage_size();
    TICOS_LOG_WARN("Event storage (%d) smaller than largest reset reason (%d)",
                      (int)storage_max_size, (int)worst_case_size_needed);
    return TICOS_REBOOT_TRACKING_STORAGE_TOO_SMALL;
  }

  ticos_reboot_tracking_clear_reset_info();
  return 0;
}
