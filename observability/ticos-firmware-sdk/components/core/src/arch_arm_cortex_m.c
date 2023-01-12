//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <stdint.h>

#include "ticos/core/arch.h"
#include "ticos/core/compiler.h"
#include "ticos/core/platform/core.h"

#if TICOS_COMPILER_ARM

bool ticos_arch_is_inside_isr(void) {
  // We query the "Interrupt Control State Register" to determine
  // if there is an active Exception Handler
  volatile uint32_t *ICSR = (uint32_t *)0xE000ED04;
  // Bottom byte makes up "VECTACTIVE"
  return ((*ICSR & 0xff) != 0x0);
}

void ticos_arch_disable_configurable_faults(void) {
  // Clear TICOSENA, BUSFAULTENA, USGFAULTENA, SECUREFAULTENA
  //
  // This will force all faults to be routed through the HardFault Handler
  volatile uint32_t *SHCSR = (uint32_t*)0xE000ED24;
  *SHCSR &= ~((uint32_t)0x000F0000);
}

TICOS_WEAK
void ticos_platform_halt_if_debugging(void) {
  volatile uint32_t *dhcsr = (volatile uint32_t *)0xE000EDF0;
  const uint32_t c_debugen_mask = 0x1;

  if ((*dhcsr & c_debugen_mask) == 0) {
    // no debugger is attached so return since issuing a breakpoint instruction would trigger a
    // fault
    return;
  }

  // NB: A breakpoint with value 'M' (77) for easy disambiguation from other breakpoints that may
  // be used by the system.
  TICOS_BREAKPOINT(77);
}

#endif /* TICOS_COMPILER_ARM */
