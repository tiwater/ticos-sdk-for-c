//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "sdkconfig.h"

#include <string.h>

#include "ticos/core/platform/core.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/core/trace_event.h"
#include "ticos/metrics/metrics.h"
#include "ticos/ports/reboot_reason.h"

#if CONFIG_TICOS_EVENT_COLLECTION_ENABLED

static uint8_t s_event_storage[CONFIG_TICOS_EVENT_STORAGE_RAM_SIZE];

// Note: The ESP8266 noinit region appears to overlap with the heap in the bootloader so this
// region may actually get cleared across reset. In this scenario, we will still have the reset
// info from the RTC backup domain to work with.
static TICOS_PUT_IN_SECTION(".noinit.reboot_info")
    uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

void ticos_esp_port_event_collection_boot(void) {

  sResetBootupInfo reset_info;
  ticos_reboot_reason_get(&reset_info);
  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_info);

  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);
  ticos_reboot_tracking_collect_reset_info(evt_storage);


#if CONFIG_TICOS_EVENT_HEARTBEATS_ENABLED
  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);
#endif
}

#endif /* TICOS_EVENT_COLLECTION_ENABLED */
