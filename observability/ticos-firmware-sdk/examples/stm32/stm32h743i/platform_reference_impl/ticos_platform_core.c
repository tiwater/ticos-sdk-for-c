//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/core/platform/core.h"

#include "ticos/core/compiler.h"
#include "stm32h7xx_hal.h"

int ticos_platform_boot(void) {
  return 0;
}

TICOS_NORETURN void ticos_platform_reboot(void) {
  ticos_platform_halt_if_debugging();

  NVIC_SystemReset();
  TICOS_UNREACHABLE;
}
