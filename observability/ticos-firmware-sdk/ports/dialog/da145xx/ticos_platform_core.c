//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/components.h"
#include "ticos/ports/reboot_reason.h"

#include "datasheet.h"

#if !defined(TICOS_EVENT_STORAGE_RAM_SIZE)
#if defined (__DA14531__)
#define TICOS_EVENT_STORAGE_RAM_SIZE 128
#else
#define TICOS_EVENT_STORAGE_RAM_SIZE 512
#endif
#endif

int ticos_platform_boot(void) {
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
  const uint16_t tmp = (GetWord16(SYS_CTRL_REG) & ~REMAP_ADR0) | 0 | SW_RESET;
  SetWord16(SYS_CTRL_REG, tmp);

  TICOS_UNREACHABLE;
}
