//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fault handling for AARCH64 based devices

#if defined(__aarch64__)

#include "ticos/core/platform/core.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/panics/arch/arm/aarch64.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/coredump_impl.h"
#include "ticos/panics/fault_handling.h"

TICOS_WEAK
void ticos_platform_fault_handler(TICOS_UNUSED const sTcsRegState *regs,
                                     TICOS_UNUSED eTicosRebootReason reason) {}

const sTcsCoredumpRegion *ticos_coredump_get_arch_regions(size_t *num_regions) {
  *num_regions = 0;
  return NULL;
}

static eTicosRebootReason s_crash_reason = kTcsRebootReason_Unknown;

TICOS_NORETURN
void ticos_fault_handler(const sTcsRegState *regs, eTicosRebootReason reason) {
  ticos_platform_fault_handler(regs, reason);

  if (s_crash_reason == kTcsRebootReason_Unknown) {
    sTcsRebootTrackingRegInfo info = {
      .pc = regs->pc,
      .lr = regs->x[30],
    };
    ticos_reboot_tracking_mark_reset_imminent(reason, &info);
    s_crash_reason = reason;
  }

  sTicosCoredumpSaveInfo save_info = {
    .regs = regs,
    .regs_size = sizeof(*regs),
    .trace_reason = s_crash_reason,
  };

  sCoredumpCrashInfo info = {
    .stack_address = (void *)regs->sp,
    .trace_reason = save_info.trace_reason,
    .exception_reg_state = regs,
  };

  save_info.regions = ticos_platform_coredump_get_regions(&info, &save_info.num_regions);
  const bool coredump_saved = ticos_coredump_save(&save_info);
  if (coredump_saved) {
    ticos_reboot_tracking_mark_coredump_saved();
  }

  ticos_platform_reboot();
  TICOS_UNREACHABLE;
}

TICOS_NORETURN
static void prv_fault_handling_assert(void *pc, void *lr, eTicosRebootReason reason) {
  sTcsRebootTrackingRegInfo info = {
    .pc = (uint32_t)(uintptr_t)pc,
    .lr = (uint32_t)(uintptr_t)lr,
  };
  s_crash_reason = reason;
  ticos_reboot_tracking_mark_reset_imminent(s_crash_reason, &info);

  // For assert path, we will trap into fault handler
  __builtin_trap();
}

void ticos_fault_handling_assert(void *pc, void *lr) {
  prv_fault_handling_assert(pc, lr, kTcsRebootReason_Assert);
}

void ticos_fault_handling_assert_extra(void *pc, void *lr, sTicosAssertInfo *extra_info) {
  prv_fault_handling_assert(pc, lr, extra_info->assert_reason);
}

size_t ticos_coredump_storage_compute_size_required(void) {
  // actual values don't matter since we are just computing the size
  sTcsRegState core_regs = {0};
  sTicosCoredumpSaveInfo save_info = {
    .regs = &core_regs,
    .regs_size = sizeof(core_regs),
    .trace_reason = kTcsRebootReason_UnknownError,
  };

  sCoredumpCrashInfo info = {
    // we'll just pass the current stack pointer, value shouldn't matter
    .stack_address = (void *)&core_regs,
    .trace_reason = save_info.trace_reason,
    .exception_reg_state = NULL,
  };
  save_info.regions = ticos_platform_coredump_get_regions(&info, &save_info.num_regions);

  return ticos_coredump_get_save_size(&save_info);
}

#endif /* __aarch64__ */
