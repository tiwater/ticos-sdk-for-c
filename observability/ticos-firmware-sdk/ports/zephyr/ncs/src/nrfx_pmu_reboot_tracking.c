//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Power management unit" (PMU)'s "reset reason" (RESETREAS) Register.
//!
//! More details can be found in the "RESETREAS" section in an NRF reference manual

#include "ticos/ports/reboot_reason.h"

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/sdk_assert.h"

#include <nrfx.h>
#include <hal/nrf_power.h>

//! Note: Nordic uses different headers for reset reason information depending on platform. NRF52 &
//! NRF91 reasons are defined in nrf_power.h whereas nrf53 reasons are defined in nrf_reset.h
#if !NRF_POWER_HAS_RESETREAS
#include <hal/nrf_reset.h>
#endif

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

#if NRF_POWER_HAS_RESETREAS
static eTicosRebootReason prv_decode_power_resetreas(uint32_t reset_cause) {
  eTicosRebootReason reset_reason;
  if (reset_cause & NRF_POWER_RESETREAS_RESETPIN_MASK) {
    TICOS_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kTcsRebootReason_PinReset;
  } else if (reset_cause & NRF_POWER_RESETREAS_DOG_MASK) {
    TICOS_PRINT_RESET_INFO(" Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & NRF_POWER_RESETREAS_SREQ_MASK) {
    TICOS_PRINT_RESET_INFO(" Software");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & NRF_POWER_RESETREAS_LOCKUP_MASK) {
    TICOS_PRINT_RESET_INFO(" Lockup");
    reset_reason = kTcsRebootReason_Lockup;
#if defined (POWER_RESETREAS_LPCOMP_Msk)
  } else if (reset_cause & NRF_POWER_RESETREAS_LPCOMP_MASK) {
    TICOS_PRINT_RESET_INFO(" LPCOMP Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
#endif
  } else if (reset_cause & NRF_POWER_RESETREAS_DIF_MASK) {
    TICOS_PRINT_RESET_INFO(" Debug Interface Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
#if defined (NRF_POWER_RESETREAS_VBUS_MASK)
  } else if (reset_cause & NRF_POWER_RESETREAS_VBUS_MASK) {
    TICOS_PRINT_RESET_INFO(" VBUS Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
#endif
  } else if (reset_cause == 0) {
    // absence of a value, means a power on reset took place
    TICOS_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else {
    TICOS_PRINT_RESET_INFO(" Unknown");
    reset_reason = kTcsRebootReason_Unknown;
  }
  return reset_reason;
}
#else
static eTicosRebootReason prv_decode_reset_resetreas(uint32_t reset_cause) {
  eTicosRebootReason reset_reason;
  if (reset_cause & NRF_RESET_RESETREAS_RESETPIN_MASK) {
    TICOS_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kTcsRebootReason_PinReset;
  } else if (reset_cause & NRF_RESET_RESETREAS_DOG0_MASK) {
    TICOS_PRINT_RESET_INFO(" Watchdog 0");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & NRF_RESET_RESETREAS_CTRLAP_MASK) {
    TICOS_PRINT_RESET_INFO(" Debugger");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & NRF_RESET_RESETREAS_SREQ_MASK) {
    TICOS_PRINT_RESET_INFO(" Software");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & NRF_RESET_RESETREAS_LOCKUP_MASK) {
    TICOS_PRINT_RESET_INFO(" Lockup");
    reset_reason = kTcsRebootReason_Lockup;
  } else if (reset_cause & NRF_RESET_RESETREAS_OFF_MASK) {
    TICOS_PRINT_RESET_INFO(" GPIO Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else if (reset_cause & NRF_RESET_RESETREAS_LPCOMP_MASK) {
    TICOS_PRINT_RESET_INFO(" LPCOMP Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else if (reset_cause & NRF_RESET_RESETREAS_DIF_MASK) {
    TICOS_PRINT_RESET_INFO(" Debug Interface Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
#if NRF_RESET_HAS_NETWORK
  } else if (reset_cause & NRF_RESET_RESETREAS_LSREQ_MASK) {
    TICOS_PRINT_RESET_INFO(" Software (Network)");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & NRF_RESET_RESETREAS_LLOCKUP_MASK) {
    TICOS_PRINT_RESET_INFO(" Lockup (Network)");
    reset_reason = kTcsRebootReason_Lockup;
  } else if (reset_cause & NRF_RESET_RESETREAS_LDOG_MASK) {
    TICOS_PRINT_RESET_INFO(" Watchdog (Network)");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & NRF_RESET_RESETREAS_MFORCEOFF_MASK) {
    TICOS_PRINT_RESET_INFO(" Force off (Network)");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & NRF_RESET_RESETREAS_LCTRLAP_MASK) {
    TICOS_PRINT_RESET_INFO(" Debugger (Network)");
    reset_reason = kTcsRebootReason_SoftwareReset;
#endif
  } else if (reset_cause & NRF_RESET_RESETREAS_VBUS_MASK) {
    TICOS_PRINT_RESET_INFO(" VBUS Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else if (reset_cause & NRF_RESET_RESETREAS_DOG1_MASK) {
    TICOS_PRINT_RESET_INFO(" Watchdog 1");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & NRF_RESET_RESETREAS_NFC_MASK) {
    TICOS_PRINT_RESET_INFO(" NFC Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else if (reset_cause == 0) {
    // absence of a value, means a power on reset took place
    TICOS_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else {
    TICOS_PRINT_RESET_INFO(" Unknown");
    reset_reason = kTcsRebootReason_Unknown;
  }

  return reset_reason;
}
#endif

TICOS_WEAK
void ticos_reboot_reason_get(sResetBootupInfo *info) {
  TICOS_SDK_ASSERT(info != NULL);

#if NRF_POWER_HAS_RESETREAS
  volatile uint32_t *resetreas_reg = &NRF_POWER->RESETREAS;
#else
  volatile uint32_t *resetreas_reg = &NRF_RESET->RESETREAS;
#endif

  const uint32_t reset_cause = *resetreas_reg;

  TICOS_LOG_INFO("Reset Reason, RESETREAS=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  eTicosRebootReason reset_reason;
#if NRF_POWER_HAS_RESETREAS
  reset_reason = prv_decode_power_resetreas(reset_cause);
#else
  reset_reason = prv_decode_reset_resetreas(reset_cause);
#endif

#if CONFIG_TICOS_CLEAR_RESET_REG
  *resetreas_reg |= reset_cause;
#endif

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
