//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Test functions for exercising ticos functionality.
//!

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Trigger an immediate heartbeat capture (instead of waiting for timer
// to expire)
int ticos_test_heartbeat(int argc, char *argv[]);

// Trigger a trace
int ticos_test_trace(int argc, char *argv[]);

// Trigger a user initiated reboot and confirm reason is persisted
int ticos_test_reboot(int argc, char *argv[]);

// Test different crash types where a coredump should be captured
int ticos_test_assert(int argc, char *argv[]);
int ticos_test_fault(void);
int ticos_test_hang(int argc, char *argv[]);

// Dump Ticos data collected to console
int ticos_test_export(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
