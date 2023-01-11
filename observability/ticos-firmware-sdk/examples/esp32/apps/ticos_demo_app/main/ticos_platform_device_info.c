//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Reference implementation of Ticos device info API platform dependencies for the ESP32

#include <stdio.h>

#include "ticos/components.h"
#include "ticos/esp_port/version.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
  #include "esp_mac.h"
#endif

#include <string.h>

#include "esp_system.h"
#include "ticos/components.h"

#ifndef TICOS_ESP32_SOFTWARE_TYPE
  #define TICOS_ESP32_SOFTWARE_TYPE "esp32-main"
#endif

#ifndef TICOS_ESP32_HW_REVISION
  #define TICOS_ESP32_HW_REVISION "esp32-proto"
#endif

static char s_device_serial[32];

// NOTE: Some versions of the esp-idf use locking when reading mac info
// so this isn't safe to call from an interrupt
static void prv_get_device_serial(char *buf, size_t buf_len) {
  // Use the ESP32's MAC address as unique device id:
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  size_t curr_idx = 0;
  for (size_t i = 0; i < sizeof(mac); i++) {
    size_t space_left = buf_len - curr_idx;
    int bytes_written = snprintf(&buf[curr_idx], space_left, "%02X", (int)mac[i]);
    if (bytes_written < space_left) {
      curr_idx += bytes_written;
    } else {  // we are out of space, return what we got, it's NULL terminated
      return;
    }
  }
}

void ticos_platform_device_info_boot(void) {
  prv_get_device_serial(s_device_serial, sizeof(s_device_serial));
}

void ticos_platform_get_device_info(struct TicosDeviceInfo *info) {
  *info = (struct TicosDeviceInfo){
    .device_serial = s_device_serial,
    .hardware_version = TICOS_ESP32_HW_REVISION,
    .software_version = CONFIG_TICOS_ESP32_MAIN_FIRMWARE_VERSION,
    .software_type = TICOS_ESP32_SOFTWARE_TYPE,
  };
}
