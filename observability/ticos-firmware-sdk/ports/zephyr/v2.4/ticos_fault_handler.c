//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//!

#include <arch/cpu.h>
#include <fatal.h>
#include <init.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
#include <soc.h>

#include "ticos/core/compiler.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/reboot_reason_types.h"
#include "ticos/panics/arch/arm/cortex_m.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/fault_handling.h"

// By default, the Zephyr NMI handler is an infinite loop. Instead
// let's register the Ticos Exception Handler
//
// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_install_nmi_handler() {
  extern void z_NmiHandlerSet(void (*pHandler)(void));
  z_NmiHandlerSet(TICOS_EXC_HANDLER_NMI);
  return 0;
}

SYS_INIT(prv_install_nmi_handler, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

// Note: There is no header exposed for this zephyr function
extern void sys_arch_reboot(int type);

// Intercept zephyr/kernel/fatal.c:z_fatal_error()
void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf);

void __wrap_z_fatal_error(unsigned int reason, const z_arch_esf_t *esf) {
  // flush logs prior to capturing coredump & rebooting
  LOG_PANIC();

  const struct __extra_esf_info *extra_info = &esf->extra_info;
  const _callee_saved_t *callee_regs = extra_info->callee;

  // Read the "SPSEL" bit where
  //  0 = Main Stack Pointer in use prior to exception
  //  1 = Process Stack Pointer in use prior to exception
  const uint32_t exc_return = extra_info->exc_return;
  const bool msp_was_active = (exc_return & (1 << 2)) == 0;

  sTcsRegState reg = {
    .exception_frame = (void *)(msp_was_active ? extra_info->msp : callee_regs->psp),
    .r4 = callee_regs->v1,
    .r5 = callee_regs->v2,
    .r6 = callee_regs->v3,
    .r7 = callee_regs->v4,
    .r8 = callee_regs->v5,
    .r9 = callee_regs->v6,
    .r10 = callee_regs->v7,
    .r11 = callee_regs->v8,
    .exc_return = exc_return,
  };

  ticos_fault_handler(&reg, kTcsRebootReason_HardFault);
}

TICOS_WEAK
TICOS_NORETURN
void ticos_platform_reboot(void) {
  ticos_platform_halt_if_debugging();

  sys_arch_reboot(0);
  CODE_UNREACHABLE;
}
