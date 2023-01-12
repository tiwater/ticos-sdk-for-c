//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset Control Module" (RCM)'s "System Reset Status" (SRS) Register.
//!
//! See "26.4.3 System Reset Status Register" of S32K-RM for more details

#include "ticos/ports/reboot_reason.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/sdk_assert.h"

#include "device_registers.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  TICOS_SDK_ASSERT(info != NULL);

  // NB: The S32 has two reset registers:
  //   System Reset Status (SRS) which contains reset reasons for most recent boot
  //   Sticky System Reset Status (SSRS) which contains all reasons for resets since last POR
  //
  // We'll just use the non-sticky variant for the port!

  const uint32_t reset_cause = RCM->SRS;

  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  TICOS_LOG_INFO("Reset Reason, SRS=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  // From the S32K-RM:
  //
  // The reset value of this register depends on the reset source:
  //   POR (including LVD) — 0x82
  //   LVD (without POR) — 0x02
  // Other reset — a bit is set if its corresponding reset source caused the reset

  if ((reset_cause & RCM_SRS_LVD(1)) && (reset_cause & RCM_SRS_POR(1))) {
    TICOS_PRINT_RESET_INFO(" Low or High Voltage");
    reset_reason = kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & RCM_SRS_POR(1)) {
    TICOS_PRINT_RESET_INFO(" POR");
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else if (reset_cause & RCM_SRS_MDM_AP(1)) {
    TICOS_PRINT_RESET_INFO(" Debugger (AP)");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & RCM_SRS_SW(1)) {
    TICOS_PRINT_RESET_INFO(" Software");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & RCM_SRS_JTAG(1)) {
    TICOS_PRINT_RESET_INFO(" Debugger (JTAG)");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & RCM_SRS_PIN(1)) {
    TICOS_PRINT_RESET_INFO(" Reset Pin");
    reset_reason = kTcsRebootReason_ButtonReset;
  } else if (reset_cause & RCM_SRS_LOCKUP(1)) {
    TICOS_PRINT_RESET_INFO(" Lockup");
    reset_reason = kTcsRebootReason_Lockup;
  } else if (reset_cause & RCM_SRS_WDOG(1)) {
    TICOS_PRINT_RESET_INFO(" Hardware Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCM_SRS_LOL(1)) {
    TICOS_PRINT_RESET_INFO(" Loss of Lock in PLL/FLL");
    reset_reason = kTcsRebootReason_ClockFailure;
  } else if (reset_cause & RCM_SRS_LOC(1)) {
    TICOS_PRINT_RESET_INFO(" Loss of Clock");
    reset_reason = kTcsRebootReason_ClockFailure;
  }

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
