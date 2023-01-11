#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Task watchdog API.
//!
//! This module implements a task watchdog that can be used to detect stuck RTOS
//! tasks.
//!
//! Example usage:
//!
//! // Initialize the task watchdog system on system start
//! ticos_task_watchdog_init();
//!
//! // In a task loop
//! void example_task_loop(void) {
//!   while(1) {
//!     rtos_wait_for_task_to_wake_up();
//!
//!     // Example: Use the task watchdog to monitor a block of work
//!     TICOS_TASK_WATCHDOG_START(example_task_loop_channel_id);
//!     // do work that should be monitored by the watchdog
//!     TICOS_TASK_WATCHDOG_STOP(example_task_loop_channel_id);
//!
//!     // Example: Use the task watchdog to monitor a repeated operation
//!     TICOS_TASK_WATCHDOG_START(example_task_loop_channel_id);
//!     // feeding the watchdog is only necessary if in some long-running task
//!     // that runs the risk of expiring the timeout, but is not expected to be
//!     // stuck in between calls
//!     for (size_t i = 0; i < 100; i++) {
//!       TICOS_TASK_WATCHDOG_FEED(example_task_loop_channel_id);
//!       run_slow_repeated_task();
//!     }
//!     TICOS_TASK_WATCHDOG_STOP(example_task_loop_channel_id);
//!
//!     rtos_put_task_to_sleep();
//!   }
//! }
//!
//! // In either a standalone timer, or a periodic background task, call the
//! // ticos_task_watchdog_check_all function
//! void example_task_watchdog_timer_cb(void) {
//!   ticos_task_watchdog_check_all();
//! }
//!
//! // Call ticos_task_watchdog_bookkeep() in
//! // ticos_platform_coredump_save_begin()- this updates all the task
//! // channel timeouts, in the event that the task monitor wasn't able to run
//! // for some reason. Also might be appropriate to refresh other system
//! // watchdogs prior to executing the coredump save.
//! bool ticos_platform_coredump_save_begin(void) {
//!   ticos_task_watchdog_bookkeep();
//!   // may also need to refresh other watchdogs here, for example:
//!   ticos_task_watchdog_platform_refresh_callback();
//!   return true;
//! }

#include <stddef.h>

#include "ticos/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Declare the task watchdog channel IDs. These shouldn't be used directly, but
//! should be passed to the TICOS_TASK_WATCHDOG_START etc. functions with the
//! name declared in the ticos_task_watchdog_config.def file.
#define TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(channel_) kTicosTaskWatchdogChannel_##channel_,
typedef enum {
#if TICOS_TASK_WATCHDOG_ENABLE
#include "ticos_task_watchdog_config.def"
#else
  // define one channel to prevent the compiler from complaining about a zero-length array
  TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(placeholder_)
#endif
  kTicosTaskWatchdogChannel_NumChannels,
} eTicosTaskWatchdogChannel;
#undef TICOS_TASK_WATCHDOG_CHANNEL_DEFINE
#define TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(channel_) kTicosTaskWatchdogChannel_##channel_

//! Initialize (or reset) the task watchdog system. This will stop and reset all
//! internal bookkeeping for all channels.
void ticos_task_watchdog_init(void);

//! This function should be called periodically (for example, around the
//! TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS period). It checks if any task
//! watchdog channel has reached the timeout interval. If so, it will trigger a
//! task watchdog assert.
void ticos_task_watchdog_check_all(void);

//! As in `ticos_task_watchdog_check_all`, but only updates the internal
//! bookkeeping, and does not trigger any callbacks or asserts.
//!
//! Intended to be called just prior to coredump capture when the system is in
//! the fault_handler.
void ticos_task_watchdog_bookkeep(void);

//! Start a task watchdog channel. After being started, a channel will now be
//! eligible for expiration. Also resets the timeout interval for the channel.
#define TICOS_TASK_WATCHDOG_START(channel_id_) \
  ticos_task_watchdog_start(TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! Reset the timeout interval for a task watchdog channel.
#define TICOS_TASK_WATCHDOG_FEED(channel_id_) \
  ticos_task_watchdog_feed(TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! Stop a task watchdog channel. After being stopped, a channel will no longer
//! be eligible for expiration and is reset
#define TICOS_TASK_WATCHDOG_STOP(channel_id_) \
  ticos_task_watchdog_stop(TICOS_TASK_WATCHDOG_CHANNEL_DEFINE(channel_id_))

//! These functions should not be used directly, but instead through the macros
//! above.
void ticos_task_watchdog_start(eTicosTaskWatchdogChannel channel_id);
void ticos_task_watchdog_feed(eTicosTaskWatchdogChannel channel_id);
void ticos_task_watchdog_stop(eTicosTaskWatchdogChannel channel_id);

//! Optional weakly defined function to perform any additional actions during
//! `ticos_task_watchdog_check_all` when no channels have expired
void ticos_task_watchdog_platform_refresh_callback(void);

#ifdef __cplusplus
}
#endif
