//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Example populating ticos_platform_get_device_info() implementation for DA1469x

#include <stdio.h>
#include <string.h>

#include "ticos/components.h"

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (struct TicosDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_version = ticos_create_unique_version_string("1.0.0"),
    .hardware_version = "DA14695",
    .software_type = "da1469x-demo-app",
  };
}
