//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A port for recovering reset reason information by reading the
//! "Reset and Clock Control" (RCC)'s "Reset Status Register" (RSR).
//!
//! More details can be found in the "RCC Reset Status Register (RCC_RSR)"
//! section of the STM32H7 family reference manual.

#include "ticos/ports/reboot_reason.h"

#include "stm32h7xx_ll_rcc.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/sdk_assert.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

//! Mappings come from "8.4.4 Reset source identification" of
//! "STM32H742, STM32H743/753 and STM32H750" Reference Manual
typedef enum ResetSource {
  kResetSource_PwrPor = (RCC_RSR_PORRSTF | RCC_RSR_PINRSTF | RCC_RSR_BORRSTF |
                         RCC_RSR_D2RSTF | RCC_RSR_D1RSTF | RCC_RSR_CPURSTF),
  kResetSource_Pin = (RCC_RSR_PINRSTF | RCC_RSR_CPURSTF),
  kResetSource_PwrBor = (RCC_RSR_PINRSTF | RCC_RSR_BORRSTF | RCC_RSR_CPURSTF),
  kResetSource_Software = (RCC_RSR_SFTRSTF | RCC_RSR_PINRSTF | RCC_RSR_CPURSTF),
  kResetSource_Cpu = RCC_RSR_CPURSTF,
  kResetSource_Wwdg = (RCC_RSR_WWDG1RSTF | RCC_RSR_PINRSTF | RCC_RSR_CPURSTF),
  kResetSource_Iwdg = (RCC_RSR_IWDG1RSTF | RCC_RSR_PINRSTF | RCC_RSR_CPURSTF),
  kResetSource_D1Exit = RCC_RSR_D1RSTF,
  kResetSource_D2Exit = RCC_RSR_D2RSTF,
  kResetSource_LowPowerError =
      (RCC_RSR_LPWRRSTF | RCC_RSR_PINRSTF | RCC_RSR_CPURSTF),
} eResetSource;

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  const uint32_t reset_cause = RCC->RSR;

  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  TICOS_PRINT_RESET_INFO("Reset Reason, RCC_RSR=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  const uint32_t reset_mask_all =
      (RCC_RSR_IWDG1RSTF | RCC_RSR_CPURSTF | RCC_RSR_D1RSTF | RCC_RSR_D2RSTF |
       RCC_RSR_BORRSTF | RCC_RSR_PINRSTF | RCC_RSR_PORRSTF | RCC_RSR_SFTRSTF |
       RCC_RSR_WWDG1RSTF | RCC_RSR_LPWRRSTF);

  switch (reset_cause & reset_mask_all) {
    case kResetSource_PwrPor:
      TICOS_PRINT_RESET_INFO(" Power on Reset");
      reset_reason = kTcsRebootReason_PowerOnReset;
      break;
    case kResetSource_Pin:
      TICOS_PRINT_RESET_INFO(" Pin Reset");
      reset_reason = kTcsRebootReason_PinReset;
      break;
    case kResetSource_PwrBor:
      TICOS_PRINT_RESET_INFO(" Brown out");
      reset_reason = kTcsRebootReason_BrownOutReset;
      break;
    case kResetSource_Software:
      TICOS_PRINT_RESET_INFO(" Software");
      reset_reason = kTcsRebootReason_SoftwareReset;
      break;
    case kResetSource_Cpu:
      // A reset generated via RCC_AHB3RSTR
      TICOS_PRINT_RESET_INFO(" Cpu");
      reset_reason = kTcsRebootReason_SoftwareReset;
      break;
    case kResetSource_Wwdg:
      TICOS_PRINT_RESET_INFO(" Window Watchdog");
      reset_reason = kTcsRebootReason_HardwareWatchdog;
      break;
    case kResetSource_Iwdg:
      TICOS_PRINT_RESET_INFO(" Independent Watchdog");
      reset_reason = kTcsRebootReason_HardwareWatchdog;
      break;
    case kResetSource_D1Exit:
      TICOS_PRINT_RESET_INFO(" D1 Low Power Exit");
      reset_reason = kTcsRebootReason_LowPower;
      break;
    case kResetSource_D2Exit:
      TICOS_PRINT_RESET_INFO(" D2 Low Power Exit");
      reset_reason = kTcsRebootReason_LowPower;
      break;
    case kResetSource_LowPowerError:
      TICOS_PRINT_RESET_INFO(" Illegal D1 DStandby / CStop");
      reset_reason = kTcsRebootReason_UnknownError;
      break;
    default:
      TICOS_PRINT_RESET_INFO(" Unknown");
      break;
  }

#if TICOS_REBOOT_REASON_CLEAR
  // we have read the reset information so clear the bits (since they are sticky across reboots)
  __HAL_RCC_CLEAR_RESET_FLAGS();
#endif

  *info = (sResetBootupInfo) {
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}
