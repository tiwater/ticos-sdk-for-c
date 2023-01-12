//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Task watchdog implementation.

#include "ticos/core/task_watchdog.h"
#include "ticos/core/task_watchdog_impl.h"

//! non-module definitions

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/core.h"
#include "ticos/panics/assert.h"

#if TICOS_TASK_WATCHDOG_ENABLE

sTicosTaskWatchdogInfo g_ticos_task_channel_info;

//! Catch if the timeout is set to an incompatible literal
static const uint32_t s_watchdog_timeout_ms = TICOS_TASK_WATCHDOG_TIMEOUT_INTERVAL_MS;

void ticos_task_watchdog_init(void) {
  g_ticos_task_channel_info = (struct TicosTaskWatchdogInfo){0};
}

static bool prv_ticos_task_watchdog_expired(struct TicosTaskWatchdogChannel channel,
                                               uint64_t current_time_ms) {
  const bool active = channel.state == kTicosTaskWatchdogChannelState_Started;
  // const bool expired = (channel.fed_time_ms + s_watchdog_timeout_ms) < current_time_ms;
  const bool expired = (current_time_ms - channel.fed_time_ms) > s_watchdog_timeout_ms;

  return active && expired;
}

static size_t prv_ticos_task_watchdog_do_check(void) {
  const uint32_t time_since_boot_ms = ticos_platform_get_time_since_boot_ms();

  size_t expired_channels_count = 0;
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(g_ticos_task_channel_info.channels); i++) {
    if (prv_ticos_task_watchdog_expired(g_ticos_task_channel_info.channels[i],
                                           time_since_boot_ms)) {
      expired_channels_count++;
    }
  }

  return expired_channels_count;
}

void ticos_task_watchdog_check_all(void) {
  size_t expired_channels_count = prv_ticos_task_watchdog_do_check();

  // if any channel reached expiration, trigger a panic
  if (expired_channels_count > 0) {
    TICOS_SOFTWARE_WATCHDOG();
  } else {
    ticos_task_watchdog_platform_refresh_callback();
  }
}

void ticos_task_watchdog_bookkeep(void) {
  // only update the internal structure, don't trigger callbacks
  (void)prv_ticos_task_watchdog_do_check();
}

void ticos_task_watchdog_start(eTicosTaskWatchdogChannel channel_id) {
  g_ticos_task_channel_info.channels[channel_id].fed_time_ms =
    ticos_platform_get_time_since_boot_ms();
  g_ticos_task_channel_info.channels[channel_id].state =
    kTicosTaskWatchdogChannelState_Started;
}

void ticos_task_watchdog_feed(eTicosTaskWatchdogChannel channel_id) {
  g_ticos_task_channel_info.channels[channel_id].fed_time_ms =
    ticos_platform_get_time_since_boot_ms();
}

void ticos_task_watchdog_stop(eTicosTaskWatchdogChannel channel_id) {
  g_ticos_task_channel_info.channels[channel_id].state =
    kTicosTaskWatchdogChannelState_Stopped;
}

//! Callback which is called when there are no expired tasks; can be used for
//! example to reset a hardware watchdog
TICOS_WEAK void ticos_task_watchdog_platform_refresh_callback(void) {}

#else  // TICOS_TASK_WATCHDOG_ENABLE

void ticos_task_watchdog_bookkeep(void) {}

#endif  // TICOS_TASK_WATCHDOG_ENABLE
