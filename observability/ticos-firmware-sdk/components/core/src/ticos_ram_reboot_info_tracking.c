//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A RAM-backed implementation used for tracking state across system reboots. More details about
//! how to use the API can be found in reboot_tracking.h
//! Assumptions:
//!  - RAM state survives across resets (this is generally true as long as power is stable)
//!    If power is lost, nothing will fail but the reboot will not be recorded
//!  - The memory which needs to persist in RAM must _not_ be initialized by any of the firmwares
//!    upon reboot & the memory must be placed in the same region for the firmwares running on the
//!    system (i.e bootloader & main image).

#include "ticos/core/reboot_tracking.h"
#include "ticos_reboot_tracking_private.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "ticos/core/compiler.h"
#include "ticos/core/errors.h"

#define TICOS_REBOOT_INFO_MAGIC 0x21544252

#define TICOS_REBOOT_INFO_VERSION 2

#define TICOS_REBOOT_REASON_NOT_SET 0xffffffff

typedef TICOS_PACKED_STRUCT TcsRebootInfo {
  //! A cheap way to check if the data within the struct is valid
  uint32_t magic;
  //! Version of the struct. If a new field is added it should be appended right before rsvd. This
  //! way we can remain backwards compatible but know what fields are valid.
  uint8_t version;
  //! The number of times the system has reset due to an error
  //! without any crash data being read out via the Ticos packetizer
  uint8_t crash_count;
  uint8_t rsvd1[1];
  uint8_t coredump_saved;
  uint32_t last_reboot_reason; // eTicosRebootReason or TICOS_REBOOT_REASON_NOT_SET
  uint32_t pc;
  uint32_t lr;
  //! Most MCUs have a register which reveals why a device rebooted.
  //!
  //! This can be particularly useful for debugging reasons for unexpected reboots
  //! (where no coredump was saved or no user initiated reset took place). Examples
  //! of this include brown out resets (BORs) & hardware watchdog resets.
  uint32_t reset_reason_reg0;
  // Reserved for future additions
  uint32_t rsvd2[10];
} sTcsRebootInfo;

TICOS_STATIC_ASSERT(sizeof(sTcsRebootInfo) == TICOS_REBOOT_TRACKING_REGION_SIZE,
                       "struct doesn't match expected size");

static sTcsRebootInfo *s_tcs_reboot_info;

//! Struct to retrieve reboot reason data from. Matches the fields of sTcsRebootReason
//! as documented in reboot_tracking.h
typedef struct {
  eTicosRebootReason reboot_reg_reason;
  eTicosRebootReason prior_stored_reason;
  bool is_valid;
} sTcsRebootReasonData;

// Private struct to store reboot reason after reboot tracking is initialized
static sTcsRebootReasonData s_reboot_reason_data = {
  .is_valid = false,
};

static bool prv_check_or_init_struct(void) {
  if (s_tcs_reboot_info == NULL) {
    return false;
  }

  if (s_tcs_reboot_info->magic == TICOS_REBOOT_INFO_MAGIC) {
    return true;
  }

  // structure doesn't match what we expect, reset it
  *s_tcs_reboot_info = (sTcsRebootInfo) {
    .magic = TICOS_REBOOT_INFO_MAGIC,
    .version = TICOS_REBOOT_INFO_VERSION,
    .last_reboot_reason = TICOS_REBOOT_REASON_NOT_SET,
  };
  return true;
}

static bool prv_read_reset_info(sTcsResetReasonInfo *info) {
  if ((s_tcs_reboot_info->last_reboot_reason == TICOS_REBOOT_REASON_NOT_SET) &&
      (s_tcs_reboot_info->reset_reason_reg0 == 0)) {
    return false; // no reset crashes!
  }

  *info = (sTcsResetReasonInfo) {
    .reason = (eTicosRebootReason)s_tcs_reboot_info->last_reboot_reason,
    .pc = s_tcs_reboot_info->pc,
    .lr = s_tcs_reboot_info->lr,
    .reset_reason_reg0 = s_tcs_reboot_info->reset_reason_reg0,
    .coredump_saved = s_tcs_reboot_info->coredump_saved == 1,
  };

  return true;
}

//! Records reboot reasons from reboot register and prior saved reboot
//!
//! Stores both the new reboot reason derived from a platform's reboot register and
//! any previously saved reboot reason. If there is no previously stored reboot reason,
//! the reboot register reason is used.
//!
//! @param reboot_reg_reason New reboot reason from this boot
//! @param prior_stored_reason Prior reboot reason stored in s_tcs_reboot_info
static void prv_record_reboot_reason(eTicosRebootReason reboot_reg_reason,
                                     eTicosRebootReason prior_stored_reason) {
  s_reboot_reason_data.reboot_reg_reason = reboot_reg_reason;

  if (prior_stored_reason != (eTicosRebootReason)TICOS_REBOOT_REASON_NOT_SET) {
    s_reboot_reason_data.prior_stored_reason = prior_stored_reason;
  } else {
    s_reboot_reason_data.prior_stored_reason = reboot_reg_reason;
  }

  s_reboot_reason_data.is_valid = true;
}

static bool prv_get_unexpected_reboot_occurred(void) {
  // Check prior_stored_reason, reboot is unexpected if prior reason is set and in error range or
  // unknown
  if (s_reboot_reason_data.prior_stored_reason !=
      (eTicosRebootReason)TICOS_REBOOT_REASON_NOT_SET) {
    if (s_reboot_reason_data.prior_stored_reason == kTcsRebootReason_Unknown ||
        s_reboot_reason_data.prior_stored_reason >= kTcsRebootReason_UnknownError) {
      return true;
    }
  }

  // Check reboot_reg_reason second, reboot is unexpected if in error range or unknown
  return (s_reboot_reason_data.reboot_reg_reason == kTcsRebootReason_Unknown ||
          s_reboot_reason_data.reboot_reg_reason >= kTcsRebootReason_UnknownError);
}

static void prv_record_reboot_event(eTicosRebootReason reboot_reason,
                                    const sTcsRebootTrackingRegInfo *reg) {
  // Store both the new reason reported by hardware and the current recorded reason
  // The combination of these will be used to determine if the bootup was expected
  // by the metrics subsystem
  // s_tcs_reboot_info can be cleared by any call to ticos_reboot_tracking_collect_reset_info
  prv_record_reboot_reason(reboot_reason, s_tcs_reboot_info->last_reboot_reason);

  if (s_tcs_reboot_info->last_reboot_reason != TICOS_REBOOT_REASON_NOT_SET) {
    // we are already tracking a reboot. We don't overwrite this because generally the first reboot
    // in a loop reveals what started the crash loop
    return;
  }
  s_tcs_reboot_info->last_reboot_reason = reboot_reason;
  if (reg == NULL) { // we don't have any extra metadata
    return;
  }

  s_tcs_reboot_info->pc = reg->pc;
  s_tcs_reboot_info->lr = reg->lr;
}

void ticos_reboot_tracking_boot(
    void *start_addr, const sResetBootupInfo *bootup_info) {
  s_tcs_reboot_info = start_addr;

  if (start_addr == NULL) {
    return;
  }

  if (!prv_check_or_init_struct()) {
    return;
  }

  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;
  if (bootup_info != NULL) {
    s_tcs_reboot_info->reset_reason_reg0 = bootup_info->reset_reason_reg;
    reset_reason = bootup_info->reset_reason;
  }

  prv_record_reboot_event(reset_reason, NULL);

  if (prv_get_unexpected_reboot_occurred()) {
    s_tcs_reboot_info->crash_count++;
  }
}

void ticos_reboot_tracking_mark_reset_imminent(eTicosRebootReason reboot_reason,
                                                  const sTcsRebootTrackingRegInfo *reg) {
  if (!prv_check_or_init_struct()) {
    return;
  }

  prv_record_reboot_event(reboot_reason, reg);
}

bool ticos_reboot_tracking_read_reset_info(sTcsResetReasonInfo *info) {
  if (info == NULL) {
    return false;
  }

  if (!prv_check_or_init_struct()) {
    return false;
  }

  return prv_read_reset_info(info);
}

void ticos_reboot_tracking_reset_crash_count(void) {
  if (!prv_check_or_init_struct()) {
    return;
  }

  s_tcs_reboot_info->crash_count = 0;
}

size_t ticos_reboot_tracking_get_crash_count(void) {
  if (!prv_check_or_init_struct()) {
    return 0;
  }

  return s_tcs_reboot_info->crash_count;
}

void ticos_reboot_tracking_clear_reset_info(void) {
  if (!prv_check_or_init_struct()) {
    return;
  }

  s_tcs_reboot_info->last_reboot_reason = TICOS_REBOOT_REASON_NOT_SET;
  s_tcs_reboot_info->coredump_saved = 0;
  s_tcs_reboot_info->pc = 0;
  s_tcs_reboot_info->lr = 0;
  s_tcs_reboot_info->reset_reason_reg0 = 0;
}

void ticos_reboot_tracking_mark_coredump_saved(void) {
  if (!prv_check_or_init_struct()) {
    return;
  }

  s_tcs_reboot_info->coredump_saved = 1;
}

int ticos_reboot_tracking_get_reboot_reason(sTcsRebootReason *reboot_reason) {
  if (reboot_reason == NULL || !s_reboot_reason_data.is_valid) {
    return -1;
  }

  *reboot_reason = (sTcsRebootReason){
    .reboot_reg_reason = s_reboot_reason_data.reboot_reg_reason,
    .prior_stored_reason = s_reboot_reason_data.prior_stored_reason,
  };

  return 0;
}

int ticos_reboot_tracking_get_unexpected_reboot_occurred(bool *unexpected_reboot_occurred) {
  if (unexpected_reboot_occurred == NULL || !s_reboot_reason_data.is_valid) {
    return -1;
  }

  *unexpected_reboot_occurred = prv_get_unexpected_reboot_occurred();
  return 0;
}

void ticos_reboot_tracking_clear_reboot_reason(void) {
  s_reboot_reason_data = (sTcsRebootReasonData){
    .is_valid = false,
  };
}
