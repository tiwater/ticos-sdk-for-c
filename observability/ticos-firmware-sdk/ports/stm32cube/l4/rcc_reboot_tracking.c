//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset and Clock Control" (RCC)'s "control & status register" (CSR) Register.
//!
//! More details can be found in the "RCC clock control & status register (RCC_CSR)"
//! section of the STM32L4 family reference manual.

#include "ticos/ports/reboot_reason.h"

#include "stm32l4xx_ll_rcc.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/sdk_assert.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  TICOS_SDK_ASSERT(info != NULL);

  const uint32_t reset_cause = RCC->CSR;

  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  TICOS_LOG_INFO("Reset Reason, RCC_CSR=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET) {
    TICOS_PRINT_RESET_INFO(" Standby Wakeup");
    reset_reason = kTcsRebootReason_DeepSleep;

    // clear the standy wakeup
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
  } else if (reset_cause & LL_RCC_CSR_FWRSTF) {
    TICOS_PRINT_RESET_INFO(" Firewall Reset");
    reset_reason = kTcsRebootReason_UsageFault;
  } else if (reset_cause & LL_RCC_CSR_SFTRSTF) {
    TICOS_PRINT_RESET_INFO(" Software");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & LL_RCC_CSR_IWDGRSTF) {
    TICOS_PRINT_RESET_INFO(" Independent Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & LL_RCC_CSR_WWDGRSTF) {
    TICOS_PRINT_RESET_INFO(" Window Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & LL_RCC_CSR_BORRSTF) {
    TICOS_PRINT_RESET_INFO(" Brown out / POR");
    // The STM32L4 doesn't have a way to disambiguate
    // a BOR vs POR
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else if (reset_cause & LL_RCC_CSR_PINRSTF) {
    TICOS_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kTcsRebootReason_PinReset;
  } else if (reset_cause & LL_RCC_CSR_OBLRSTF) {
    TICOS_PRINT_RESET_INFO(" Option Byte Loader");
    // Unexpected reset type, we'll just classify as Unknown
  } else {
    TICOS_PRINT_RESET_INFO(" Unknown");
  }

  // we have read the reset information so clear the bits (since they are sticky across reboots)
  __HAL_RCC_CLEAR_RESET_FLAGS();

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
