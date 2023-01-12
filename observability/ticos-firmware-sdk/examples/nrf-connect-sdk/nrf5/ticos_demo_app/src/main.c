//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Example console main

#include <drivers/hwinfo.h>
#include <logging/log.h>
#include <ticos/components.h>
#include <ticos_ncs.h>
#include <stdio.h>

#include "ticos/ports/watchdog.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

//! wrapper to enclose logic around log_strdup
char *prv_conditional_log_strdup(char *str) {
#if !TICOS_ZEPHYR_VERSION_GT(3, 1)
  #if defined(CONFIG_LOG) && !defined(CONFIG_LOG2)
  // Before zephyr 3.1, LOG was a different option from LOG2 and required
  // manually duplicating string argument values. Only required if CONFIG_LOG is
  // in use.
  return log_strdup(str);
  #endif
#endif

  return str;
}
static void prv_set_device_id(void) {
  uint8_t dev_id[16] = {0};
  char dev_id_str[sizeof(dev_id) * 2 + 1];
  char *dev_str = "UNKNOWN";

  // Obtain the device id
  ssize_t length = hwinfo_get_device_id(dev_id, sizeof(dev_id));

  // If this fails for some reason, use a fixed ID instead
  if (length <= 0) {
    dev_str = CONFIG_SOC_SERIES "-test";
    length = strlen(dev_str);
  } else {
    // Render the obtained serial number in hexadecimal representation
    for (size_t i = 0; i < length; i++) {
      (void)snprintf(&dev_id_str[i * 2], sizeof(dev_id_str), "%02x", dev_id[i]);
    }
    dev_str = dev_id_str;
  }

  LOG_INF("Device ID: %s", prv_conditional_log_strdup(dev_str));

  ticos_ncs_device_id_set(dev_str, length * 2);
}

#define WD_FEED_THREAD_STACK_SIZE 500
// set priority to lowest application thread; shell_uart, where the 'tcs test
// hang' command runs from, uses the same priority by default, so this should
// not preempt it and correctly trip the watchdog
#if CONFIG_SHELL_THREAD_PRIORITY_OVERRIDE
  #error "Watchdog feed thread priority must be lower than shell thread priority"
#endif
#define WD_FEED_THREAD_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

static void prv_wd_feed_thread_function(void *arg0, void *arg1, void *arg2) {
  ARG_UNUSED(arg0);
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);

  while (1) {
    ticos_software_watchdog_feed();
    k_sleep(K_SECONDS(1));
  }
}
K_THREAD_DEFINE(wd_feed_thread, WD_FEED_THREAD_STACK_SIZE, prv_wd_feed_thread_function, NULL, NULL,
                NULL, WD_FEED_THREAD_PRIORITY, 0, 0);

static void prv_start_watchdog_feed_thread(void) {
  LOG_INF("starting watchdog feed thread 🐶");
  ticos_software_watchdog_enable();
  k_thread_name_set(wd_feed_thread, "wd_feed_thread");
  k_thread_start(wd_feed_thread);
}

void main(void) {
  LOG_INF("Booting Ticos sample app!");

  // Set the device id based on the hardware UID
  prv_set_device_id();

  ticos_device_info_dump();

  prv_start_watchdog_feed_thread();
}
