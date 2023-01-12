//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Command definitions for the minimal shell/console implementation.

#include "ticos/demo/shell_commands.h"

#include <stddef.h>

#include "ticos/core/compiler.h"
#include "ticos/core/data_export.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/demo/cli.h"

static int prv_panics_component_required(void) {
  TICOS_LOG_RAW("Disabled. panics component integration required");
  return -1;
}

TICOS_WEAK
int ticos_demo_cli_cmd_get_core(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

TICOS_WEAK
int ticos_demo_cli_cmd_clear_core(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

TICOS_WEAK
int ticos_demo_cli_cmd_crash(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  return prv_panics_component_required();
}

int ticos_demo_cli_cmd_export(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  ticos_data_export_dump_chunks();

  return 0;
}

static const sTicosShellCommand s_ticos_shell_commands[] = {
  {"clear_core", ticos_demo_cli_cmd_clear_core, "Clear an existing coredump"},
  {"drain_chunks",  ticos_demo_drain_chunk_data, "Flushes queued Ticos data. To upload data see https://ticos.io/posting-chunks-with-gdb"},
  {"export", ticos_demo_cli_cmd_export, "Export base64-encoded chunks. To upload data see https://ticos.io/chunk-data-export"},
  {"get_core", ticos_demo_cli_cmd_get_core, "Get coredump info"},
  {"get_device_info", ticos_demo_cli_cmd_get_device_info, "Get device info"},

  //
  // Test commands for validating SDK functionality: https://ticos.io/mcu-test-commands
  //

  {"test_assert",  ticos_demo_cli_cmd_assert, "Trigger ticos assert"},

#if TICOS_COMPILER_ARM
  {"test_busfault",  ticos_demo_cli_cmd_busfault, "Trigger a busfault"},
  {"test_hardfault",  ticos_demo_cli_cmd_hardfault, "Trigger a hardfault"},
  {"test_memmanage",  ticos_demo_cli_cmd_memmanage, "Trigger a memory management fault"},
  {"test_usagefault",  ticos_demo_cli_cmd_usagefault, "Trigger a usage fault"},
#endif

  {"test_log", ticos_demo_cli_cmd_test_log, "Writes test logs to log buffer"},
  {"test_log_capture",  ticos_demo_cli_cmd_trigger_logs, "Trigger capture of current log buffer contents"},
  {"test_reboot",  ticos_demo_cli_cmd_system_reboot, "Force system reset and track it with a trace event"},
  {"test_trace",  ticos_demo_cli_cmd_trace_event_capture, "Capture an example trace event"},

  {"help", ticos_shell_help_handler, "Lists all commands"},
};

// Note: Declared as weak so an end user can override the command table
TICOS_WEAK
const sTicosShellCommand *const g_ticos_shell_commands = s_ticos_shell_commands;
TICOS_WEAK
const size_t g_ticos_num_shell_commands = TICOS_ARRAY_SIZE(s_ticos_shell_commands);
