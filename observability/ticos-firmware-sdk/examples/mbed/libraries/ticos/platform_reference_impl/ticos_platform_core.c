//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! Reference implementation of the ticos platform header for Mbed
#include "cmsis.h"

#include "ticos/core/compiler.h"

int ticos_platform_boot(void) {
  return 0;
}

TICOS_NORETURN void ticos_platform_reboot(void) {
  ticos_platform_halt_if_debugging();

  NVIC_SystemReset();
  TICOS_UNREACHABLE;
}
