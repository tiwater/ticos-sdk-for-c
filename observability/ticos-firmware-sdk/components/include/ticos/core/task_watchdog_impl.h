#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Task watchdog APIs intended for use within the ticos-firmware-sdk

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/config.h"

// Keep this after config.h
#include "ticos/core/task_watchdog.h"

#ifdef __cplusplus
extern "C" {
#endif

enum TicosTaskWatchdogChannelState {
  kTicosTaskWatchdogChannelState_Stopped = 0,
  kTicosTaskWatchdogChannelState_Started,
  kTicosTaskWatchdogChannelState_Expired,
};
struct TicosTaskWatchdogChannel {
  // Using a 32-bit value to track fed_time. This is enough for 49 days, which
  // should be much longer than any watchdog timeout.
  uint32_t fed_time_ms;
  enum TicosTaskWatchdogChannelState state;
};
typedef struct TicosTaskWatchdogInfo {
  struct TicosTaskWatchdogChannel channels[kTicosTaskWatchdogChannel_NumChannels];
} sTicosTaskWatchdogInfo;

//! Bookkeeping for the task watchdog channels. If the structure needs to change
//! in the future, rename it to keep analyzer compatibility (eg "_v2")
extern sTicosTaskWatchdogInfo g_ticos_task_channel_info;

#ifdef __cplusplus
}
#endif
