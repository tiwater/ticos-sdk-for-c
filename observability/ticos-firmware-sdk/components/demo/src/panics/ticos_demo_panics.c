//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands which require integration of the "panic" component.

#include <inttypes.h>
#include <stdlib.h>

#include "ticos/core/arch.h"
#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/platform/core.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/core/reboot_tracking.h"
#include "ticos/demo/cli.h"
#include "ticos/panics/assert.h"
#include "ticos/panics/coredump.h"
#include "ticos/panics/platform/coredump.h"
#include "ticos_demo_cli_aux_private.h"

TICOS_NO_OPT
static void do_some_work_base(char *argv[]) {
  // An assert that is guaranteed to fail. We perform
  // the check against argv so that the compiler can't
  // perform any optimizations
  TICOS_ASSERT((uint32_t)(uintptr_t)argv == 0xdeadbeef);
}

TICOS_NO_OPT
static void do_some_work1(char *argv[]) {
  do_some_work_base(argv);
}

TICOS_NO_OPT
static void do_some_work2(char *argv[]) {
  do_some_work1(argv);
}

TICOS_NO_OPT
static void do_some_work3(char *argv[]) {
  do_some_work2(argv);
}

TICOS_NO_OPT
static void do_some_work4(char *argv[]) {
  do_some_work3(argv);
}

TICOS_NO_OPT
static void do_some_work5(char *argv[]) {
  do_some_work4(argv);
}

int ticos_demo_cli_cmd_crash(int argc, char *argv[]) {
  int crash_type = 0;

  if (argc >= 2) {
    crash_type = atoi(argv[1]);
  }

  switch (crash_type) {
    case 0:
      TICOS_ASSERT(0);
      break;

    case 1:
      g_bad_func_call();
      break;

    case 2: {
      uint64_t *buf = g_ticos_unaligned_buffer;
      *buf = 0xbadcafe0000;
    } break;

    case 3:
      do_some_work5(argv);
      break;

    case 4:
      TICOS_SOFTWARE_WATCHDOG();
      break;

    default:
      // this should only ever be reached if crash_type is invalid
      TICOS_LOG_ERROR("Usage: \"crash\" or \"crash <n>\" where n is 0..4");
      return -1;
  }

  // Should be unreachable. If we get here, trigger an assert and record the crash_type which
  // failed to trigger a crash
  TICOS_ASSERT_RECORD((uint32_t)crash_type);
  return -1;
}

int ticos_demo_cli_cmd_get_core(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  size_t total_size = 0;
  if (!ticos_coredump_has_valid_coredump(&total_size)) {
    TICOS_LOG_INFO("No coredump present!");
    return 0;
  }
  TICOS_LOG_INFO("Has coredump with size: %d", (int)total_size);
  return 0;
}

int ticos_demo_cli_cmd_clear_core(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  TICOS_LOG_INFO("Invalidating coredump");
  ticos_platform_coredump_storage_clear();
  return 0;
}

int ticos_demo_cli_cmd_assert(int argc, char *argv[]) {
  // permit running with a user-provided "extra" value for testing that path
  if (argc > 1) {
    TICOS_ASSERT_RECORD(atoi(argv[1]));
  } else {
    TICOS_ASSERT(0);
  }
}

#if TICOS_COMPILER_ARM

int ticos_demo_cli_cmd_hardfault(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  ticos_arch_disable_configurable_faults();

  uint64_t *buf = g_ticos_unaligned_buffer;
  *buf = 0xdeadbeef0000;

  return -1;
}

int ticos_demo_cli_cmd_memmanage(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  // Per "Relation of the MPU to the system memory map" in ARMv7-M reference manual:
  //
  // "The MPU is restricted in how it can change the default memory map attributes associated with
  //  System space, that is, for addresses 0xE0000000 and higher. System space is always marked as
  //  XN, Execute Never."
  //
  // So we can trip a MemManage exception by simply attempting to execute any addresss >=
  // 0xE000.0000
  void (*bad_func)(void) = (void (*)(void))0xEEEEDEAD;
  bad_func();

  // We should never get here -- platforms MemManage or HardFault handler should be tripped
  return -1;
}

int ticos_demo_cli_cmd_busfault(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  void (*unaligned_func)(void) = (void (*)(void))0x50000001;
  unaligned_func();

  // We should never get here -- platforms BusFault or HardFault handler should be tripped
  // with a precise error due to unaligned execution
  return -1;
}

int ticos_demo_cli_cmd_usagefault(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  uint64_t *buf = g_ticos_unaligned_buffer;
  *buf = 0xbadcafe0000;

  // We should never get here -- platforms UsageFault or HardFault handler should be tripped due to
  // unaligned access
  return -1;
}

int ticos_demo_cli_loadaddr(int argc, char *argv[]) {
  if (argc < 2) {
    TICOS_LOG_ERROR("Usage: loadaddr <addr>");
    return -1;
  }
  uint32_t addr = (uint32_t)strtoul(argv[1], NULL, 0);
  uint32_t val = *(uint32_t *)addr;

  TICOS_LOG_INFO("Read 0x%08" PRIx32 " from 0x%08" PRIx32, val, (uint32_t)(uintptr_t)addr);
  return 0;
}

#endif /* TICOS_COMPILER_ARM */
