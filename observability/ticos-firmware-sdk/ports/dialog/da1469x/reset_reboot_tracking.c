//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

//! @file
//!
//! Map DA1469x reboot reasons to ticos definitions

#include "ticos/ports/reboot_reason.h"

#include "DA1469xAB.h"
#include "sdk_defs.h"

__RETAINED_UNINIT static uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

void ticos_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = { 0 };
  ticos_reboot_reason_get(&reset_info);
  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  eTicosRebootReason reset_reason;

  uint32_t reset_cause = CRG_TOP->RESET_STAT_REG;
  if (reset_cause && (CRG_TOP_RESET_STAT_REG_CMAC_WDOGRESET_STAT_Msk |
                      CRG_TOP_RESET_STAT_REG_SWD_HWRESET_STAT_Msk    |
                      CRG_TOP_RESET_STAT_REG_WDOGRESET_STAT_Msk      |
                      CRG_TOP_RESET_STAT_REG_SWRESET_STAT_Msk        |
                      CRG_TOP_RESET_STAT_REG_HWRESET_STAT_Msk        |
                      CRG_TOP_RESET_STAT_REG_PORESET_STAT_Msk)) {
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else if (reset_cause && CRG_TOP_RESET_STAT_REG_CMAC_WDOGRESET_STAT_Msk) {
    reset_reason = kTcsRebootReason_SoftwareWatchdog;
  } else if (reset_cause && CRG_TOP_RESET_STAT_REG_SWD_HWRESET_STAT_Msk) {
    reset_reason = kTcsRebootReason_DebuggerHalted;
  } else if (reset_cause && CRG_TOP_RESET_STAT_REG_SWRESET_STAT_Msk) {
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if(reset_cause && CRG_TOP_RESET_STAT_REG_HWRESET_STAT_Msk) {
    reset_reason = kTcsRebootReason_ButtonReset;
  } else {
    reset_reason = kTcsRebootReason_Unknown;
  }

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
