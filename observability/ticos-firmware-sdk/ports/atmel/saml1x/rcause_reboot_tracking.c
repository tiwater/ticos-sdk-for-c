//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! Reset Controller's (RSTC) "Reset Cause" (RCAUSE) register.
//!
//! More details can be found in the "RSTC – Reset Controller" of the
//! "SAM L10/L11 Family" Datasheet

#include "ticos/ports/reboot_reason.h"

#include "sam.h"

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = (uint32_t)REG_RSTC_RCAUSE;
  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  if (reset_cause & RSTC_RCAUSE_POR_Msk) {
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else if (reset_cause & RSTC_RCAUSE_BODCORE_Msk) {
    reset_reason = kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & RSTC_RCAUSE_BODVDD_Msk) {
    reset_reason = kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & RSTC_RCAUSE_EXT_Msk) {
    reset_reason = kTcsRebootReason_PinReset;
  } else if (reset_cause & RSTC_RCAUSE_WDT_Msk) {
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & RSTC_RCAUSE_SYST_Msk) {
    reset_reason = kTcsRebootReason_SoftwareReset;
  }

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
