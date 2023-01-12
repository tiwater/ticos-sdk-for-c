//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/components.h"
#include "ticos/ports/freertos.h"
#include "ticos/ports/reboot_reason.h"

#include "hw_cpm_da1469x.h"

#ifndef TICOS_EVENT_STORAGE_RAM_SIZE
#define TICOS_EVENT_STORAGE_RAM_SIZE 1024
#endif

int ticos_platform_boot(void) {
  ticos_freertos_port_boot();
  ticos_platform_reboot_tracking_boot();
  ticos_build_info_dump();
  ticos_device_info_dump();

  static uint8_t s_event_storage[TICOS_EVENT_STORAGE_RAM_SIZE];
  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);

  ticos_reboot_tracking_collect_reset_info(evt_storage);

  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  return 0;
}

TICOS_NORETURN void ticos_platform_reboot(void) {
  hw_cpm_reboot_system();
  TICOS_UNREACHABLE;
}
