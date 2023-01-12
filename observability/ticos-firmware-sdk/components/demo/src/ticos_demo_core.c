//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands used by demo applications to exercise the Ticos SDK

#include "ticos/demo/cli.h"

#include <stdlib.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/device_info.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/core/reboot_tracking.h"

int ticos_demo_cli_cmd_get_device_info(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  ticos_device_info_dump();
  return 0;
}

int ticos_demo_cli_cmd_system_reboot(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  void *pc;
  TICOS_GET_PC(pc);
  void *lr;
  TICOS_GET_LR(lr);
  sTcsRebootTrackingRegInfo reg_info = {
    .pc = (uint32_t)(uintptr_t)pc,
    .lr = (uint32_t)(uintptr_t)lr,
  };

  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_UserReset, &reg_info);
  ticos_platform_reboot();
  return 0;
}
