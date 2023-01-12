//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Reads the reset reason information saved in the ESP8266 RTC backup domain.
//! I believe this info itself is copied by the bootloader from another register but
//! could not find any documentation about it.

#include "ticos/ports/reboot_reason.h"

#include "sdkconfig.h"
#include "esp_system.h"
#include "internal/esp_system_internal.h"
#include "esp8266/rtc_register.h"
#include "esp8266/rom_functions.h"
#include "esp_libc.h"

#include "ticos/core/debug_log.h"
#include "ticos/config.h"

#if TICOS_ENABLE_REBOOT_DIAG_DUMP
#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)
#else
#define TICOS_PRINT_RESET_INFO(...)
#endif

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  eTicosRebootReason reboot_reason = kTcsRebootReason_Unknown;
  int esp_reset_cause = (int)esp_reset_reason();

  TICOS_LOG_INFO("ESP Reset Cause 0x%" PRIx32, esp_reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  switch (esp_reset_cause) {
    case ESP_RST_POWERON:
      reboot_reason = kTcsRebootReason_PowerOnReset;
      TICOS_PRINT_RESET_INFO(" Power On Reset");
      break;
    case ESP_RST_SW:
      reboot_reason = kTcsRebootReason_SoftwareReset;
      TICOS_PRINT_RESET_INFO(" Software Reset");
      break;
    case ESP_RST_INT_WDT:
      reboot_reason = kTcsRebootReason_HardwareWatchdog;
      TICOS_PRINT_RESET_INFO(" INT Watchdog");
      break;
    case ESP_RST_TASK_WDT:
      reboot_reason = kTcsRebootReason_HardwareWatchdog;
      TICOS_PRINT_RESET_INFO(" Task Watchdog");
      break;
    case ESP_RST_WDT:
      // Empirically, once set it seems this state is sticky across resets until a POR takes place
      // so we don't automatically flag it as a watchdog
      TICOS_PRINT_RESET_INFO(" Hardware Watchdog");
      break;
    case ESP_RST_DEEPSLEEP:
      reboot_reason = kTcsRebootReason_DeepSleep;
      TICOS_PRINT_RESET_INFO(" Deep Sleep");
      break;
    case ESP_RST_BROWNOUT:
      reboot_reason = kTcsRebootReason_BrownOutReset;
      TICOS_PRINT_RESET_INFO(" Brown Out");
      break;
    case ESP_RST_PANIC:
      reboot_reason = kTcsRebootReason_HardFault;
      TICOS_PRINT_RESET_INFO(" Software Panic");
      break;
    default:
      TICOS_PRINT_RESET_INFO(" Unknown");
      reboot_reason = kTcsRebootReason_UnknownError;
      break;
  }

  *info = (sResetBootupInfo) {
    .reset_reason_reg = esp_reset_cause,
    .reset_reason = reboot_reason,
  };
}
