//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/core/platform/core.h"

#include <init.h>
#include <kernel.h>
#include <soc.h>

#include "ticos/components.h"
#include "ticos/ports/reboot_reason.h"
#include "zephyr_release_specific_headers.h"

#if !TICOS_ZEPHYR_VERSION_GT(2, 5)

// Note: CONFIG_OPENOCD_SUPPORT was deprecated in Zephyr 2.6 and fully removed in the 3.x line. To
// maintain backward compatibility with older versions of Zephyr we inline the check here.
//
// When support for Zephyr <= 2.5 is removed, we should adopt an approach like the following
//  https://github.com/ticos/ticos-firmware-sdk/pull/26

#if !CONFIG_OPENOCD_SUPPORT
#error "CONFIG_OPENOCD_SUPPORT=y must be added to your prj.conf"
#endif

#endif

#if CONFIG_TICOS_METRICS
#include "ticos/metrics/metrics.h"
#endif

#if CONFIG_TICOS_CACHE_FAULT_REGS
// Zephy's z_arm_fault() function consumes and clears
// the SCB->CFSR register so we must wrap their function
// so we can preserve the pristine fault register values.
void __wrap_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                        _callee_saved_t *callee_regs) {

  ticos_coredump_cache_fault_regs();

  // Now let the Zephyr fault handler complete as normal.
  void __real_z_arm_fault(uint32_t msp, uint32_t psp, uint32_t exc_return,
                          _callee_saved_t *callee_regs);
  __real_z_arm_fault(msp, psp, exc_return, callee_regs);
}
#endif

uint64_t ticos_platform_get_time_since_boot_ms(void) {
  return k_uptime_get();
}

//! Provide a strong implementation of assert_post_action for Zephyr's built-in
//! __ASSERT() macro.
#if CONFIG_TICOS_CATCH_ZEPHYR_ASSERT
#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
  ARG_UNUSED(file);
  ARG_UNUSED(line);
#endif
  TICOS_ASSERT(0);
}
#endif

// On boot-up, log out any information collected as to why the
// reset took place

TICOS_PUT_IN_SECTION(".noinit.tcs_reboot_info")
static uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];

static uint8_t s_event_storage[CONFIG_TICOS_EVENT_STORAGE_SIZE];

#if !CONFIG_TICOS_REBOOT_REASON_GET_CUSTOM
TICOS_WEAK
void ticos_reboot_reason_get(sResetBootupInfo *info) {
  *info = (sResetBootupInfo) {
    .reset_reason = kTcsRebootReason_Unknown,
  };
}
#endif

// Note: the function signature has changed here across zephyr releases
// "struct device *dev" -> "const struct device *dev"
//
// Since we don't use the arguments we match anything with () to avoid
// compiler warnings and share the same bootup logic
static int prv_init_and_log_reboot() {
  sResetBootupInfo reset_info = { 0 };
  ticos_reboot_reason_get(&reset_info);

  ticos_reboot_tracking_boot(s_reboot_tracking, &reset_info);

  const sTicosEventStorageImpl *evt_storage =
      ticos_events_storage_boot(s_event_storage, sizeof(s_event_storage));
  ticos_reboot_tracking_collect_reset_info(evt_storage);
  ticos_trace_event_boot(evt_storage);


#if CONFIG_TICOS_METRICS
  sTicosMetricBootInfo boot_info = {
    .unexpected_reboot_count = ticos_reboot_tracking_get_crash_count(),
  };
  ticos_metrics_boot(evt_storage, &boot_info);
#endif

  ticos_build_info_dump();
  return 0;
}

#if CONFIG_TICOS_HEAP_STATS && CONFIG_HEAP_MEM_POOL_SIZE > 0
extern void *__real_k_malloc(size_t size);
extern void __real_k_free(void *ptr);

void *__wrap_k_malloc(size_t size) {
  void *ptr = __real_k_malloc(size);

  // Only call into heap stats from non-ISR context
  // Heap stats requires holding a lock
  if (!k_is_in_isr()) {
    TICOS_HEAP_STATS_MALLOC(ptr, size);
  }

  return ptr;
}
void __wrap_k_free(void *ptr) {
  // Only call into heap stats from non-ISR context
  // Heap stats requires holding a lock
  if (!k_is_in_isr()) {
    TICOS_HEAP_STATS_FREE(ptr);
  }
  __real_k_free(ptr);
}
#endif

SYS_INIT(prv_init_and_log_reboot,
#if CONFIG_TICOS_INIT_LEVEL_POST_KERNEL
         POST_KERNEL,
#else
         APPLICATION,
#endif
         CONFIG_TICOS_INIT_PRIORITY);
