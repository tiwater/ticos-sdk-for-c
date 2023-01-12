//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the "Reset and
//! Clock Control" (RCC)'s "Control & Status Register" (CSR).
//!
//! More details can be found in the "RCC clock control & status register
//! (RCC_CSR)" section of the STM32F7 family reference manual.

#include "ticos/components.h"
#include "stm32f7xx_hal.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

//! Reset reason codes come from "5.3.21 RCC clock control & status register
//! (RCC_CSR)" of the ST "RM0410" Reference Manual for (STM32F76xxx and
//! STM32F77xxx).
void ticos_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RCC->CSR;

  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  TICOS_PRINT_RESET_INFO("Reset Reason, RCC_CSR=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  // look for the first bit set in the reset cause register.
  //
  // pin reset is checked last; all other internally generated resets are wired
  // to the reset pin, see section 5.1.2 of the Reference Manual.
  if (reset_cause & RCC_CSR_SFTRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Software");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & RCC_CSR_PORRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Power on Reset");
    reset_reason = kTcsRebootReason_PowerOnReset;
  } else if (reset_cause & RCC_CSR_BORRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Brown out");
    reset_reason = kTcsRebootReason_BrownOutReset;
  } else if (reset_cause & RCC_CSR_WWDGRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Window Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_CSR_IWDGRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Independent Watchdog");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & RCC_CSR_LPWRRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Low Power");
    reset_reason = kTcsRebootReason_LowPower;
  } else if (reset_cause & RCC_CSR_PINRSTF_Msk) {
    TICOS_PRINT_RESET_INFO(" Pin Reset");
    reset_reason = kTcsRebootReason_PinReset;
  } else {
    TICOS_PRINT_RESET_INFO(" Unknown");
    reset_reason = kTcsRebootReason_Unknown;
  }

#if TICOS_REBOOT_REASON_CLEAR
  // we have read the reset information so clear the bits (since they are sticky
  // across reboots)
  __HAL_RCC_CLEAR_RESET_FLAGS();
#endif

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
