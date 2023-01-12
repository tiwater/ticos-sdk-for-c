//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(tcs_app, LOG_LEVEL_DBG);

#include <string.h>

#include "ticos/components.h"

#include "ticos/ports/zephyr/http.h"

sTcsHttpClientConfig g_tcs_http_client_config = {
  .api_key = "<YOUR PROJECT KEY HERE>",
};

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (sTicosDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "zephyr-app",
    .software_version = "1.0.0-dev",
    .hardware_version = CONFIG_BOARD,
  };
}

void main(void) {
  LOG_INF("Ticos Demo App! Board %s\n", CONFIG_BOARD);
  ticos_device_info_dump();
  ticos_zephyr_port_install_root_certs();
}
