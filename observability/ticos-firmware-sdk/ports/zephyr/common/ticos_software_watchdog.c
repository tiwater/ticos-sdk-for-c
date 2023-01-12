//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A Software Watchdog implementation backed by the Zephyr timer subsystem
//!
//! The implementation uses the k_timer_ module. When the timer expires,
//! the implementation will assert so a coredump of the system state is captured.

#include "ticos/panics/assert.h"
#include "ticos/ports/watchdog.h"

#include <zephyr.h>

static void prv_software_watchdog_timeout(struct k_timer *dummy) {
  TICOS_SOFTWARE_WATCHDOG();
}

K_TIMER_DEFINE(s_watchdog_timer, prv_software_watchdog_timeout, NULL);
static uint32_t s_software_watchog_timeout_ms = TICOS_WATCHDOG_SW_TIMEOUT_SECS * 1000;

static void prv_start_or_reset(uint32_t timeout_ms) {
  k_timer_start(&s_watchdog_timer, K_MSEC(timeout_ms), K_MSEC(timeout_ms));
}

int ticos_software_watchdog_enable(void) {
  prv_start_or_reset(s_software_watchog_timeout_ms);
  return 0;
}

int ticos_software_watchdog_disable(void) {
  k_timer_stop(&s_watchdog_timer);
  return 0;
}

int ticos_software_watchdog_feed(void) {
  prv_start_or_reset(s_software_watchog_timeout_ms);
  return 0;
}

int ticos_software_watchdog_update_timeout(uint32_t timeout_ms) {
  s_software_watchog_timeout_ms = timeout_ms;
  prv_start_or_reset(s_software_watchog_timeout_ms);
  return 0;
}
