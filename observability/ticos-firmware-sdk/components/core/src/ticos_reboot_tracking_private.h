#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal utilities used for tracking reboot reasons

#include <inttypes.h>
#include <stdbool.h>

#include "ticos/core/reboot_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TcsResetReasonInfo {
  eTicosRebootReason reason;
  uint32_t pc;
  uint32_t lr;
  uint32_t reset_reason_reg0;
  bool coredump_saved;
} sTcsResetReasonInfo;

//! Clears any crash information which was stored
void ticos_reboot_tracking_clear_reset_info(void);

//! Clears stored reboot reason stored at bootup
void ticos_reboot_tracking_clear_reboot_reason(void);

bool ticos_reboot_tracking_read_reset_info(sTcsResetReasonInfo *info);

#ifdef __cplusplus
}
#endif
