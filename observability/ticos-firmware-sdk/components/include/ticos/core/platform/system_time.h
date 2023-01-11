#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Dependency functions which can optionally be implemented for time tracking within the Ticos
//! SDK.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kTicosCurrentTimeType_Unknown = 0,
  //! the number of seconds that have elapsed since the Unix epoch,
  //! (00:00:00 UTC on 1 January 1970)
  kTicosCurrentTimeType_UnixEpochTimeSec = 1,
} eTicosCurrentTimeType;

typedef struct {
  eTicosCurrentTimeType type;
  union {
    uint64_t unix_timestamp_secs;
  } info;
} sTicosCurrentTime;

//! Returns the current system time
//!
//! This dependency can (optionally) be implemented if a device keeps track of time and wants to track the
//! exact time events occurred on device. If no time is provided, the Ticos backend will
//! automatically create a timestamp based on when an event is received by the chunks endpoint.
//!
//! @note By default, a weak version of this function is implemented which always returns false (no
//! time available)
//!
//! @param return true if a time could be recovered, false otherwise
bool ticos_platform_time_get_current(sTicosCurrentTime *time);

#ifdef __cplusplus
}
#endif
