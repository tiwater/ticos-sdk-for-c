//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset Management Unit" (RMU)'s "Reset Cause" (RSTCAUSE) Register.
//!
//! More details can be found in the "RMU_RSTCAUSE Register" of the reference manual
//! for the specific EFR or EFM chip family. It makes use of APIs that are part of
//! the Gecko SDK.

#include "ticos/ports/reboot_reason.h"

#include "em_rmu.h"
#include "em_emu.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/sdk_assert.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

#if defined(_RMU_RSTCAUSE_MASK)
static eTicosRebootReason prv_get_and_print_reason(uint32_t reset_cause) {
  // Find the RMU_RSTCAUSE Register data sheet for the EFM/EFR part for more details
  // For example, in the EFM32PG12 it's in section "8.3.2 RMU_RSTCAUSE Register"
  //
  // Note that some reset types are shared across EFM/EFR MCU familes. For the ones
  // that are not, we wrap the reason with an ifdef.
  if (reset_cause & RMU_RSTCAUSE_PORST) {
    TICOS_PRINT_RESET_INFO(" Power on Reset");
    return kTcsRebootReason_PowerOnReset;
#if defined(RMU_RSTCAUSE_AVDDBOD)
  } else if (reset_cause & RMU_RSTCAUSE_AVDDBOD) {
    TICOS_PRINT_RESET_INFO(" AVDD Brown Out");
    return kTcsRebootReason_BrownOutReset;
#endif
#if defined(RMU_RSTCAUSE_DVDDBOD)
  } else if (reset_cause & RMU_RSTCAUSE_DVDDBOD) {
    TICOS_PRINT_RESET_INFO(" DVDD Brown Out");
    return kTcsRebootReason_BrownOutReset;
#endif
#if defined(RMU_RSTCAUSE_DECBOD)
  } else if (reset_cause & RMU_RSTCAUSE_DECBOD) {
    TICOS_PRINT_RESET_INFO(" DEC Brown Out");
    return kTcsRebootReason_BrownOutReset;
#endif
  } else if (reset_cause & RMU_RSTCAUSE_LOCKUPRST) {
    TICOS_PRINT_RESET_INFO(" Lockup");
    return kTcsRebootReason_Lockup;
  } else if (reset_cause & RMU_RSTCAUSE_SYSREQRST) {
    TICOS_PRINT_RESET_INFO(" Software");
    return kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & RMU_RSTCAUSE_WDOGRST) {
    TICOS_PRINT_RESET_INFO(" Watchdog");
    return kTcsRebootReason_HardwareWatchdog;
#if defined(RMU_RSTCAUSE_EM4WURST)
  } else if (reset_cause & RMU_RSTCAUSE_EM4RST) {
    TICOS_PRINT_RESET_INFO(" EM4 Wakeup");
    return kTcsRebootReason_DeepSleep;
#endif
  } else if (reset_cause & RMU_RSTCAUSE_EXTRST) {
    TICOS_PRINT_RESET_INFO(" Pin Reset");
    return kTcsRebootReason_ButtonReset;
  }

  TICOS_PRINT_RESET_INFO(" Unknown");
  return kTcsRebootReason_Unknown;
}
#elif defined(_EMU_RSTCTRL_MASK)

static eTicosRebootReason prv_get_and_print_reason(uint32_t reset_cause) {
  // Find the EMU_RSTCAUSE Register data sheet for the EFM/EFR part for more details
  // For example, in the EFR32xG21 it's in section "12.5.13 EMU_RSTCAUSE - Reset cause"
  //
  // Note that some reset types are shared across EFM/EFR MCU familes. For the ones
  // that are not, we wrap the reason with an ifdef.

  if (reset_cause & EMU_RSTCAUSE_POR) {
    TICOS_PRINT_RESET_INFO(" Power on Reset");
    return kTcsRebootReason_PowerOnReset;
  } else if (reset_cause & EMU_RSTCAUSE_AVDDBOD) {
    TICOS_PRINT_RESET_INFO(" AVDD Brown Out");
    return kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & EMU_RSTCAUSE_IOVDD0BOD) {
    TICOS_PRINT_RESET_INFO(" IOVDD0 Brown Out");
    return kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & EMU_RSTCAUSE_DVDDBOD) {
    TICOS_PRINT_RESET_INFO(" DVDD Brown Out");
    return kTcsRebootReason_BrownOutReset;
  } else if (reset_cause &  EMU_RSTCAUSE_DVDDLEBOD) {
    TICOS_PRINT_RESET_INFO(" DVDDLE Brown Out");
    return kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & EMU_RSTCAUSE_DECBOD) {
    TICOS_PRINT_RESET_INFO(" DEC Brown Out");
    return kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & EMU_RSTCAUSE_LOCKUP) {
    TICOS_PRINT_RESET_INFO(" Lockup");
    return kTcsRebootReason_Lockup;
  } else if (reset_cause & EMU_RSTCAUSE_SYSREQ) {
    TICOS_PRINT_RESET_INFO(" Software");
    return kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & EMU_RSTCAUSE_WDOG0) {
    TICOS_PRINT_RESET_INFO(" Watchdog 0");
    return kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & EMU_RSTCAUSE_WDOG1) {
    TICOS_PRINT_RESET_INFO(" Watchdog 1");
    return kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & EMU_RSTCAUSE_EM4) {
    TICOS_PRINT_RESET_INFO(" EM4 Wakeup");
    return kTcsRebootReason_DeepSleep;
  } else if (reset_cause & EMU_RSTCAUSE_SETAMPER) {
    TICOS_PRINT_RESET_INFO(" SE Tamper");
    return kTcsRebootReason_UnknownError;
  } else if (reset_cause & EMU_RSTCAUSE_SESYSREQ) {
    TICOS_PRINT_RESET_INFO(" SE Software Reset");
    return kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & EMU_RSTCAUSE_SELOCKUP) {
    TICOS_PRINT_RESET_INFO(" SE Lockup");
    return kTcsRebootReason_Lockup;
  } else if (reset_cause & EMU_RSTCAUSE_PIN) {
    TICOS_PRINT_RESET_INFO(" SE Pin Reset");
    return kTcsRebootReason_PinReset;
  }

  TICOS_PRINT_RESET_INFO(" Unknown");
  return kTcsRebootReason_Unknown;
}
#else
#error "Unexpected RSTCTRL configuration"
#endif

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  TICOS_SDK_ASSERT(info != NULL);

  // This routine simply reads RMU->RSTCAUSE and zero's out
  // bits that aren't relevant to the reset. For more details
  // check out the logic in ${PATH_TO_GECKO_SDK}/platform/emlib/src/em_rmu.c
  const uint32_t reset_cause = RMU_ResetCauseGet();

  TICOS_PRINT_RESET_INFO("Reset Reason, RSTCAUSE=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  eTicosRebootReason reset_reason = prv_get_and_print_reason(reset_cause);

#if TICOS_REBOOT_REASON_CLEAR
  // we have read the reset information so clear the bits (since they are sticky across reboots)
  RMU_ResetCauseClear();
#endif

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
