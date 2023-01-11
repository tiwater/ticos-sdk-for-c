#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Helpers used for serializing multiple events into a single message

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_BATCHED_EVENTS_MAX_HEADER_LENGTH 5

typedef struct {
  size_t length;
  uint8_t data[TICOS_BATCHED_EVENTS_MAX_HEADER_LENGTH];
} sTicosBatchedEventsHeader;

//! Builds the header used when events are sent in one message
//!
//! @num_events The number events that will be sent in one message
//! @header_out Populated with the header that needs to lead the events to send.
//!  If no header is needed (i.e num_events <= 1), the length can be 0.
void ticos_batched_events_build_header(
    size_t num_events, sTicosBatchedEventsHeader *header_out);

#ifdef __cplusplus
}
#endif
