//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of the ticos_platform_get_device_info platform API

#include "fake_ticos_platform_get_device_info.h"

#include "ticos/core/platform/device_info.h"

struct TicosDeviceInfo g_fake_device_info = {
    .device_serial = "DAABBCCDD",
    .software_version = "1.2.3",
    .software_type = "main",
    .hardware_version = "evt_24",
};

void ticos_platform_get_device_info(struct TicosDeviceInfo *info) {
  *info = g_fake_device_info;
}
