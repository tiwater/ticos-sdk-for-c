//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "fake_ticos_platform_time.h"

#include <stdbool.h>

typedef struct {
  bool enabled;
  sTicosCurrentTime time;
} sTicosFakeTimerState;

static sTicosFakeTimerState s_fake_timer_state;

void fake_ticos_platform_time_enable(bool enable) {
  s_fake_timer_state.enabled = enable;
}

void fake_ticos_platform_time_set(const sTicosCurrentTime *time) {
  s_fake_timer_state.time = *time;
}

//
// fake implementation
//

bool ticos_platform_time_get_current(sTicosCurrentTime *time) {
  if (!s_fake_timer_state.enabled) {
    return false;
  }

  *time = s_fake_timer_state.time;

  return true;
}
