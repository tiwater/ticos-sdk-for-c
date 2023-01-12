//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Test functions for exercising ticos functionality.
//!

#include "ticos_test.h"

#include "ticos/components.h"
#include "ticos/ports/freertos.h"

#define TICOS_CHUNK_SIZE 1500

// Triggers an immediate heartbeat capture (instead of waiting for timer
// to expire)
int ticos_test_heartbeat(int argc, char *argv[]) {
  ticos_metrics_heartbeat_debug_trigger();
  return 0;
}

int ticos_test_trace(int argc, char *argv[]) {
  TICOS_TRACE_EVENT_WITH_LOG(critical_error, "A test error trace!");
  return 0;
}

//! Trigger a user initiated reboot and confirm reason is persisted
int ticos_test_reboot(int argc, char *argv[]) {
  ticos_reboot_tracking_mark_reset_imminent(kTcsRebootReason_UserReset, NULL);
  ticos_platform_reboot();
}

//
// Test different crash types where a coredump should be captured
//

int ticos_test_assert(int argc, char *argv[]) {
  TICOS_ASSERT(0);
  return -1;  // should never get here
}

int ticos_test_fault(void) {
  void (*bad_func)(void) = (void *)0xEEEEDEAD;
  bad_func();
  return -1;  // should never get here
}

int ticos_test_hang(int argc, char *argv[]) {
  while (1) {
  }
  return -1;  // should never get here
}

// Dump Ticos data collected to console
int ticos_test_export(int argc, char *argv[]) {
  ticos_data_export_dump_chunks();
  return 0;
}
