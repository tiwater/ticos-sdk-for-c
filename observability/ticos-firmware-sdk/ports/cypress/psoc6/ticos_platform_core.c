//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Implementation of core dependencies and setup for PSOC6 based products that are using the
//! ModusToolbox SDK

#include "ticos/components.h"
#include "ticos/ports/reboot_reason.h"
#include "ticos/ports/freertos.h"

#if TICOS_PORT_MEMORY_TRACKING_ENABLED
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)
#include <malloc.h>
#endif
#endif /* TICOS_PORT_MEMORY_TRACKING_ENABLED */

#if TICOS_PORT_WIFI_TRACKING_ENABLED
#include "cy_wcm.h"
#endif /* TICOS_PORT_WIFI_TRACKING_ENABLED */

#ifndef TICOS_EVENT_STORAGE_RAM_SIZE
#define TICOS_EVENT_STORAGE_RAM_SIZE 1024
#endif

#ifndef TICOS_LOG_STORAGE_RAM_SIZE
  #define TICOS_LOG_STORAGE_RAM_SIZE 512
#endif

#if TICOS_PORT_WIFI_TRACKING_ENABLED
static cy_wcm_wlan_statistics_t s_last_wlan_statistics = { 0 };
#endif

void ticos_metrics_heartbeat_port_collect_data(void) {
#if TICOS_PORT_MEMORY_TRACKING_ENABLED
#if defined(__GNUC__) && !defined(__ARMCC_VERSION)

  struct mallinfo mall_info = mallinfo();

  // linker symbols for location of heap
  extern uint32_t __HeapBase;
  extern uint32_t __HeapLimit;

  const uint32_t heap_total_size = (uint32_t)&__HeapLimit - (uint32_t)&__HeapBase;

  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_TotalSize),
                                          heap_total_size);

  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_MinBytesFree),
                                          heap_total_size - mall_info.arena);

  ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Heap_BytesFree),
                                          heap_total_size - mall_info.uordblks);
#endif /* defined(__GNUC__) && !defined(__ARMCC_VERSION) */
#endif /* TICOS_PORT_MEMORY_TRACKING_ENABLED */

#if TICOS_PORT_WIFI_TRACKING_ENABLED
  cy_wcm_associated_ap_info_t ap_info;
  cy_rslt_t result = cy_wcm_get_associated_ap_info(&ap_info);
  if (result == CY_RSLT_SUCCESS) {
    // Note: RSSI in dBm. (<-90=Very poor, >-30=Excellent)
    // A good way to determine whether or not issues an device is facing are due to poor signal strength
    ticos_metrics_heartbeat_set_signed(TICOS_METRICS_KEY(Wifi_SignalStrength),
                                          ap_info.signal_strength);

    ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Wifi_Channel),
                                            ap_info.channel);
  }

  cy_wcm_wlan_statistics_t curr_stat;
  result = cy_wcm_get_wlan_statistics(CY_WCM_INTERFACE_TYPE_STA, &curr_stat);
  if (result == CY_RSLT_SUCCESS) {
    ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Wifi_TxBytes),
                                            curr_stat.tx_bytes - s_last_wlan_statistics.tx_bytes);
    ticos_metrics_heartbeat_set_unsigned(TICOS_METRICS_KEY(Wifi_TxBytes),
                                            curr_stat.rx_bytes - s_last_wlan_statistics.rx_bytes);
    s_last_wlan_statistics = curr_stat;
  }
#endif /* TICOS_PORT_WIFI_TRACKING_ENABLED */
}


#if TICOS_PORT_WIFI_TRACKING_ENABLED

static void prv_wcm_event_cb(cy_wcm_event_t event, cy_wcm_event_data_t *event_data) {
  switch (event) {
    case CY_WCM_EVENT_CONNECTING:
      ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(Wifi_ConnectingTime));
      break;

    case CY_WCM_EVENT_CONNECTED:
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Wifi_ConnectingTime));
      ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(Wifi_ConnectedTime));
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Wifi_ConnectCount), 1);
      break;

    case CY_WCM_EVENT_CONNECT_FAILED:
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Wifi_ConnectFailureCount), 1);
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Wifi_ConnectingTime));
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Wifi_ConnectedTime));
      break;

    case CY_WCM_EVENT_RECONNECTED:
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Wifi_ReconnectCount), 1);
      ticos_metrics_heartbeat_timer_start(TICOS_METRICS_KEY(Wifi_ConnectedTime));
      break;

    case CY_WCM_EVENT_DISCONNECTED:
      ticos_metrics_heartbeat_timer_stop(TICOS_METRICS_KEY(Wifi_ConnectedTime));
      ticos_metrics_heartbeat_add(TICOS_METRICS_KEY(Wifi_DisconnectCount), 1);
      break;

    case CY_WCM_EVENT_IP_CHANGED:
      break;

    case CY_WCM_EVENT_INITIATED_RETRY:
      break;

    case CY_WCM_EVENT_STA_JOINED_SOFTAP:
      break;

    case CY_WCM_EVENT_STA_LEFT_SOFTAP:
      break;

    default:
      break;
  }
}

void ticos_wcm_metrics_boot(void) {
  cy_wcm_register_event_callback(&prv_wcm_event_cb);
}

#endif /* TICOS_PORT_WIFI_TRACKING_ENABLED */

void ticos_metrics_heartbeat_collect_data(void) {
  ticos_metrics_heartbeat_port_collect_data();
}


TICOS_WEAK
void ticos_platform_reboot(void) {
  NVIC_SystemReset();
  while (1) { } // unreachable
}

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  static const struct {
    uint32_t start_addr;
    size_t length;
  } s_mcu_mem_regions[] = {
    {.start_addr = CY_SRAM_BASE, .length = CY_SRAM_SIZE},
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

int ticos_platform_boot(void) {
  ticos_build_info_dump();
  ticos_device_info_dump();

  ticos_freertos_port_boot();
  ticos_platform_reboot_tracking_boot();

  static uint8_t s_event_storage[TICOS_EVENT_STORAGE_RAM_SIZE];
  static uint8_t s_log_storage[TICOS_LOG_STORAGE_RAM_SIZE];
  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_trace_event_boot(evt_storage);

  ticos_reboot_tracking_collect_reset_info(evt_storage);

  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);

  ticos_log_boot(s_log_storage, sizeof(s_log_storage));

  TICOS_LOG_INFO("Ticos Initialized!");

  return 0;
}
