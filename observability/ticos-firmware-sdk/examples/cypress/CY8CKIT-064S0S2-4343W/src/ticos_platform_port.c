//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Glue layer between the Ticos SDK and the underlying platform
//!

#include <stdbool.h>

#include <FreeRTOSConfig.h>
#include <core_cm4.h>
#include <cy_device_headers.h>
#include <cy_retarget_io.h>
#include <cy_syslib.h>

#include "ticos/components.h"
#include "ticos/core/compiler.h"
#include "ticos/ports/freertos.h"
#include "ticos/ports/reboot_reason.h"
#include "ticos_platform_log_config.h"

#define TICOS_PRINT_RESET_INFO(...) TICOS_LOG_INFO(__VA_ARGS__)

typedef struct {
  uint32_t start_addr;
  size_t length;
} sMemRegions;

sMemRegions s_mcu_mem_regions[] = {
  {.start_addr = 0x08030000, .length = 0xB7000},
};

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  // IMPORTANT: All strings returned in info must be constant
  // or static as they will be used _after_ the function returns
  // See https://ticos.io/version-nomenclature for more context
  *info = (sTicosDeviceInfo){
    // An ID that uniquely identifies the device in your fleet
    // (i.e serial number, mac addr, chip id, etc)
    // Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
    .device_serial = "DEMOSERIAL",
    // A name to represent the firmware running on the MCU.
    // (i.e "ble-fw", "main-fw", or a codename for your project)
    .software_type = "app-fw",
    // The version of the "software_type" currently running.
    // "software_type" + "software_version" must uniquely represent
    // a single binary
    .software_version = "1.0.0",
    // The revision of hardware for the device. This value must remain
    // the same for a unique device.
    // (i.e evt, dvt, pvt, or rev1, rev2, etc)
    // Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
    .hardware_version = "dvt1",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void ticos_platform_reboot(void) {
  NVIC_SystemReset();
  while (1) {
  }  // unreachable
}

//! If device does not track time, return false, else return true if time is valid
bool ticos_platform_time_get_current(sTicosCurrentTime *time) {
  *time = (sTicosCurrentTime){
    .type = kTicosCurrentTimeType_UnixEpochTimeSec,
    .info = {.unix_timestamp_secs = 0},
  };
  return false;
}

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return TICOS_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

void ticos_reboot_reason_get(sResetBootupInfo *info) {
  TICOS_SDK_ASSERT(info != NULL);

  const uint32_t reset_cause = Cy_SysLib_GetResetReason();
  eTicosRebootReason reset_reason = kTcsRebootReason_Unknown;

  TICOS_LOG_INFO("Reset Reason, GetResetReason=0x%" PRIx32, reset_cause);
  TICOS_PRINT_RESET_INFO("Reset Causes: ");

  if (reset_cause & CY_SYSLIB_RESET_HWWDT) {
    TICOS_PRINT_RESET_INFO(" Watchdog Timer Reset");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_ACT_FAULT) {
    TICOS_PRINT_RESET_INFO(" Fault Logging System Active Reset Request");
    reset_reason = kTcsRebootReason_UnknownError;
  } else if (reset_cause & CY_SYSLIB_RESET_DPSLP_FAULT) {
    TICOS_PRINT_RESET_INFO(" Fault Logging System Deep-Sleep Reset Request");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else if (reset_cause & CY_SYSLIB_RESET_SOFT) {
    TICOS_PRINT_RESET_INFO(" Software Reset");
    reset_reason = kTcsRebootReason_SoftwareReset;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT0) {
    TICOS_PRINT_RESET_INFO(" MCWDT0 Reset");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT1) {
    TICOS_PRINT_RESET_INFO(" MCWDT1 Reset");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT2) {
    TICOS_PRINT_RESET_INFO(" MCWDT2 Reset");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_SWWDT3) {
    TICOS_PRINT_RESET_INFO(" MCWDT3 Reset");
    reset_reason = kTcsRebootReason_HardwareWatchdog;
  } else if (reset_cause & CY_SYSLIB_RESET_HIB_WAKEUP) {
    TICOS_PRINT_RESET_INFO(" Hibernation Exit Reset");
    reset_reason = kTcsRebootReason_DeepSleep;
  } else {
    TICOS_PRINT_RESET_INFO(" Other Error");
    reset_reason = kTcsRebootReason_Unknown;
  }

  Cy_SysLib_ClearResetReason();

  *info = (sResetBootupInfo){
    .reset_reason_reg = reset_cause,
    .reset_reason = reset_reason,
  };
}

void ticos_platform_log(eTicosPlatformLogLevel level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char log_buf[128];
  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  const char *lvl_str;
  switch (level) {
    case kTicosPlatformLogLevel_Debug:
      lvl_str = "D";
      break;

    case kTicosPlatformLogLevel_Info:
      lvl_str = "I";
      break;

    case kTicosPlatformLogLevel_Warning:
      lvl_str = "W";
      break;

    case kTicosPlatformLogLevel_Error:
      lvl_str = "E";
      break;

    default:
      lvl_str = "D";
      break;
  }

  vsnprintf(log_buf, sizeof(log_buf), fmt, args);

  printf("[%s] Tcs: %s\n", lvl_str, log_buf);
}

TICOS_PUT_IN_SECTION(".noinit.tcs_reboot_info")
static uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

void ticos_platform_reboot_tracking_boot(void) {
  sResetBootupInfo reset_info = {0};
  ticos_reboot_reason_get(&reset_info);
  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_info);
}

int ticos_platform_boot(void) {
  ticos_freertos_port_boot();

  ticos_build_info_dump();
  ticos_device_info_dump();
  ticos_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[1024];
  const sTicosEventStorageImpl *evt_storage =
    ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);

  ticos_reboot_tracking_collect_reset_info(evt_storage);

  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  TICOS_LOG_INFO("Ticos Initialized!");

  return 0;
}
