//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A minimal example app for exercising the Ticos SDK with the DA1469x SDK.

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "osal.h"
#include "resmgmt.h"
#include "hw_cpm.h"
#include "hw_gpio.h"
#include "hw_watchdog.h"
#include "sys_clock_mgr.h"
#include "sys_power_mgr.h"
#include "console.h"

#include "ticos/components.h"

static int prv_send_char(char c) {
  printf("%c", c);
  return 0;
}

static int prv_test_storage(TICOS_UNUSED int argc,
                            TICOS_UNUSED char *argv[]) {
  GLOBAL_INT_DISABLE();
  ticos_coredump_storage_debug_test_begin();
  GLOBAL_INT_RESTORE();
  ticos_coredump_storage_debug_test_finish();
  return 0;
}

static int prv_export_data(TICOS_UNUSED int argc,
                           TICOS_UNUSED char *argv[]) {
  ticos_data_export_dump_chunks();
  return 0;
}

static const sTicosShellCommand s_ticos_shell_commands[] = {
  {"crash", ticos_demo_cli_cmd_crash, ""},
  {"trace", ticos_demo_cli_cmd_trace_event_capture, ""},
  {"get_core", ticos_demo_cli_cmd_get_core, ""},
  {"clear_core", ticos_demo_cli_cmd_clear_core, ""},
  {"test_storage", prv_test_storage, ""},
  {"get_device_info", ticos_demo_cli_cmd_get_device_info, ""},
  {"reboot", ticos_demo_cli_cmd_system_reboot, ""},
  {"export", prv_export_data, ""},
  {"help", ticos_shell_help_handler, ""},
};

const sTicosShellCommand *const g_ticos_shell_commands = s_ticos_shell_commands;
const size_t g_ticos_num_shell_commands = TICOS_ARRAY_SIZE(s_ticos_shell_commands);

static void cli_task(void *pvParameters) {
  //! We will use the Ticos CLI shell as a very basic debug interface
  const sTicosShellImpl impl = {
    .send_char = prv_send_char,
  };
  ticos_demo_shell_boot(&impl);

  while (1) {
    char rx_byte;
    console_read(&rx_byte, 1);
    ticos_demo_shell_receive_char(rx_byte);
  }
}

static void periph_init(void) {}

static void prvSetupHardware(void) {
  pm_system_init(periph_init);
}

static OS_TASK s_main_task_hdl;

static void system_init(void *pvParameters) {
  OS_TASK task_h = NULL;

  cm_sys_clk_init(sysclk_XTAL32M);

  cm_apb_set_clock_divider(apb_div1);
  cm_ahb_set_clock_divider(ahb_div1);
  cm_lp_clk_init();

  /* Prepare the hardware to run this demo. */
  prvSetupHardware();

#if defined CONFIG_RETARGET
  extern void retarget_init(void);
  retarget_init();
#endif
  ticos_platform_boot();

  // Configure sleep modes
  pm_sleep_mode_set(pm_mode_extended_sleep);
  pm_set_sys_wakeup_mode(pm_sys_wakeup_mode_fast);

  OS_TASK_CREATE("CLI", cli_task, NULL, 1024 * OS_STACK_WORD_SIZE,
                 OS_TASK_PRIORITY_NORMAL, task_h);
  OS_ASSERT(task_h);

  OS_TASK_DELETE(s_main_task_hdl);
}

int main(void) {
  OS_BASE_TYPE status;

  status = OS_TASK_CREATE("SysInit",
                          system_init, (void *)0, 1024 * OS_STACK_WORD_SIZE,
                          OS_TASK_PRIORITY_HIGHEST, s_main_task_hdl);
  OS_ASSERT(status == OS_TASK_CREATE_SUCCESS);

  vTaskStartScheduler();

  // Scheduler should have started. If we get here an error took place
  // such as not enough memory to allocate required contexts in FreeRTOS heap
  TICOS_ASSERT(0);
}

void vApplicationMallocFailedHook(void) {
  TICOS_ASSERT(0);
}

void vApplicationIdleHook(void) {}
void vApplicationTickHook(void) {}
