//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Glue layer between the Ticos SDK and the underlying platform

#include "app_util_platform.h"
#include "nrf.h"

#include "ticos/components.h"
#include "ticos/ports/reboot_reason.h"

#if !defined(CONFIG_TICOS_EVENT_STORAGE_SIZE)
#define CONFIG_TICOS_EVENT_STORAGE_SIZE 512
#endif

#if !defined(CONFIG_TICOS_LOGGING_RAM_SIZE)
#define CONFIG_TICOS_LOGGING_RAM_SIZE 512
#endif

static void prv_get_device_serial(char *buf, size_t buf_len) {
  // We will use the 64bit NRF "Device identifier" as the serial number
  const size_t nrf_uid_num_words = 2;

  size_t curr_idx = 0;
  for (size_t i = 0; i < nrf_uid_num_words; i++) {
    uint32_t lsw = NRF_FICR->DEVICEID[i];

    const size_t bytes_per_word =  4;
    for (size_t j = 0; j < bytes_per_word; j++) {

      size_t space_left = buf_len - curr_idx;
      uint8_t val = (lsw >> (j * 8)) & 0xff;
      size_t bytes_written = snprintf(&buf[curr_idx], space_left, "%02X", (int)val);
      if (bytes_written < space_left) {
        curr_idx += bytes_written;
      } else { // we are out of space, return what we got, it's NULL terminated
        return;
      }
    }
  }
}

void ticos_platform_get_device_info(struct TicosDeviceInfo *info) {
  static char s_device_serial[32];
  static bool s_init = false;

  if (!s_init) {
    prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
    s_init = true;
  }

  *info = (struct TicosDeviceInfo) {
    .device_serial = s_device_serial,
    .hardware_version = "pca10056",
    .software_version = "1.0.0-dev",
    .software_type = "nrf-main",
  };
}

//! Last function called after a coredump is saved. Should perform
//! any final cleanup and then reset the device
void ticos_platform_reboot(void) {
  NVIC_SystemReset();
  TICOS_UNREACHABLE;
}

bool ticos_platform_time_get_current(sTicosCurrentTime *time) {
  //! RTC is not configured so don't capture time on device
  return false;
}

int ticos_platform_boot(void) {
  // static RAM storage where logs will be stored. Storage can be any size
  // you want but you will want it to be able to hold at least a couple logs.
  static uint8_t s_tcs_log_buf_storage[CONFIG_TICOS_LOGGING_RAM_SIZE];
  ticos_log_boot(s_tcs_log_buf_storage, sizeof(s_tcs_log_buf_storage));

  ticos_build_info_dump();
  ticos_device_info_dump();
  ticos_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[CONFIG_TICOS_EVENT_STORAGE_SIZE];
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
