//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "bsp.h"

#include "qf_port.h"
#include "qf.h"

#include "ticos/components.h"

void ticos_platform_get_device_info(sTicosDeviceInfo *info) {
  *info = (sTicosDeviceInfo) {
    .device_serial = "DEMOSERIAL",
    .software_type = "qp-main",
    .software_version = "1.0.0",
    .hardware_version = "stm32f407g-disc1",
  };
}

sTcsHttpClientConfig g_tcs_http_client_config = {
    .api_key = "<YOUR PROJECT KEY HERE>",
};

size_t ticos_platform_sanitize_address_range(void *start_addr, size_t desired_size) {
  // These symbols are defined by the linker
  extern uint32_t __sram_start;
  extern uint32_t __sram_end;

  uint32_t sram_start_address = (uint32_t) &__sram_start;
  uint32_t sram_end_address = (uint32_t) &__sram_end;

  if (((uint32_t)start_addr >= sram_start_address) && (sram_start_address < sram_end_address)) {
    return TICOS_MIN(desired_size, sram_end_address - sram_start_address);
  }

  return 0;
}

void main(void) {
  bsp_init();

  TICOS_LOG_INFO("Ticos QP demo app started...");

  ticos_build_info_dump();
  ticos_device_info_dump();

  ticos_demo_shell_boot(&(sTicosShellImpl){
      .send_char = bsp_send_char_over_uart,
  });

  QF_init();
  QF_run();
}

void QF_onStartup(void) {
}

void QF_onCleanup(void) {
}

void QV_onIdle(void) {
  char c;
  if (bsp_read_char_from_uart(&c)) {
    ticos_demo_shell_receive_char(c);
  }
}

// void Q_onAssert(char const *module, int loc) {
//   NOTE: After applying the ports/qp/qassert.h.patch and ports/qp/qf_pkg.h.patch files from the Ticos Firmware SDK,
//   the Q_onAssert function will no longer be necessary. Instead, Ticos's assertion handling will be used.
//   To ensure that no code relies on Q_onAssert, we recommend removing the Q_onAssert function and replace any usage
//   of it with the TICOS_ASSERT macro from ticos/panics/assert.h.
// }
