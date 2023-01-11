//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Example STM32H7 specific routines for populating version data.

#include <stdio.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/platform/device_info.h"


#ifndef TICOS_NRF_MAIN_SOFTWARE_VERSION
#  define TICOS_NRF_MAIN_SOFTWARE_VERSION "1.0.0"
#endif

#ifndef TICOS_NRF_SOFTWARE_TYPE
#  define TICOS_NRF_SOFTWARE_TYPE "stm32-main"
#endif

#ifndef TICOS_NRF_HW_REVISION
#  define TICOS_NRF_HW_REVISION "nucleo-h743zi2"
#endif

#define STM32_UID_LENGTH (96 / 8)
static char prv_int_to_ascii_hex(uint8_t val) {
  return val < 10 ? (char)val + '0' : (char)(val - 10) + 'A';
}

// For demo purposes, use the STM32 96-bit UID as the device serial number
static char *prv_get_device_serial(void) {
  static char s_device_serial[STM32_UID_LENGTH * 2 + 1];
  const uint8_t *uid_base = (uint8_t *)0x1FF1E800;

  for (size_t i = 0; i < STM32_UID_LENGTH ; i++) {
    const uint8_t val = uid_base[i];
    const int curr_idx = i * 2;
    s_device_serial[curr_idx] = prv_int_to_ascii_hex((val >> 4) & 0xf);
    s_device_serial[curr_idx + 1] = prv_int_to_ascii_hex(val & 0xf);
  }

  return s_device_serial;
}

void ticos_platform_get_device_info(struct TicosDeviceInfo *info) {
  *info = (struct TicosDeviceInfo) {
    .device_serial = prv_get_device_serial(),
    .hardware_version = TICOS_NRF_HW_REVISION,
    .software_version = TICOS_NRF_MAIN_SOFTWARE_VERSION,
    .software_type = TICOS_NRF_SOFTWARE_TYPE,
  };
}
