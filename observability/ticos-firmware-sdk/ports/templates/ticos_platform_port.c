//! @file
//!
//! Copyright 2022 Ticos, Inc
//!
//! Licensed under the Apache License, Version 2.0 (the "License");
//! you may not use this file except in compliance with the License.
//! You may obtain a copy of the License at
//!
//!     http://www.apache.org/licenses/LICENSE-2.0
//!
//! Unless required by applicable law or agreed to in writing, software
//! distributed under the License is distributed on an "AS IS" BASIS,
//! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//! See the License for the specific language governing permissions and
//! limitations under the License.
//!
//! Glue layer between the Ticos SDK and the underlying platform
//!
//! TODO: Fill in FIXMEs below for your platform

#include "ticos/components.h"
#include "ticos/ports/reboot_reason.h"

#include <stdbool.h>

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  // !FIXME: Populate with platform device information

  // IMPORTANT: All strings returned in info must be constant
  // or static as they will be used _after_ the function returns

  // See https://ticos.io/version-nomenclature for more context
  *info = (sTicosDeviceInfo) {
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
  // !FIXME: Perform any final system cleanup here

  // !FIXME: Reset System
  // NVIC_SystemReset()
  while (1) { } // unreachable
}

bool ticos_platform_time_get_current(sTicosCurrentTime *time) {
  // !FIXME: If the device tracks real time, update 'unix_timestamp_secs' with seconds since epoch
  // This will cause events logged by the SDK to be timestamped on the device rather than when they
  // arrive on the server
  *time = (sTicosCurrentTime) {
    .type = kTicosCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 0
    },
  };

  // !FIXME: If device does not track time, return false, else return true if time is valid
  return false;
}

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    // !FIXME: Update with list of valid memory banks to collect in a coredump
    {.start_addr = 0x00000000, .length = 0xFFFFFFFF},
  };

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(s_mcu_mem_regions); i++) {
    const uint32_t lower_addr = s_mcu_mem_regions[i].start_addr;
    const uint32_t upper_addr = lower_addr + s_mcu_mem_regions[i].length;
    if ((uint32_t)start_addr >= lower_addr && ((uint32_t)start_addr < upper_addr)) {
      return TICOS_MIN(desired_size, upper_addr - (uint32_t)start_addr);
    }
  }

  return 0;
}

//! !FIXME: This function _must_ be called by your main() routine prior
//! to starting an RTOS or baremetal loop.
int ticos_platform_boot(void) {
  // !FIXME: Add init to any platform specific ports here.
  // (This will be done in later steps in the getting started Guide)

  ticos_build_info_dump();
  ticos_device_info_dump();
  ticos_platform_reboot_tracking_boot();

  // initialize the event storage buffer
  static uint8_t s_event_storage[1024];
  const sTicosEventStorageImpl *evt_storage =
    ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));

  // configure trace events to store into the buffer
  ticos_trace_event_boot(evt_storage);

  // record the current reboot reason
  ticos_reboot_tracking_collect_reset_info(evt_storage);

  // configure the metrics component to store into the buffer
  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  TICOS_LOG_INFO("Ticos Initialized!");

  return 0;
}
