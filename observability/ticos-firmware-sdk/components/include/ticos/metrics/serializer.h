#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Heartbeat metrics are collected at a periodic interval
//! (TICOS_METRICS_HEARTBEAT_INTERVAL_SECS). Each time they are collected, the data is
//! serialized out into a compressed format to be sent up to the Ticos cloud via the "Data
//! Packetizer" (see data_packetizer.h). The utilities in this module deal with this serialization
//! process

#include <stdbool.h>
#include <stddef.h>

#include "ticos/core/event_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Compute the worst case number of bytes required to serialize Ticos data
//!
//! @return the worst case amount of space needed to serialize an event
size_t ticos_metrics_heartbeat_compute_worst_case_storage_size(void);


//! Serialize out the current set of heartbeat metrics
//!
//! @return True if the data was successfully serialized, else false if there was not enough space
//! to serialize the data
bool ticos_metrics_heartbeat_serialize(const sTicosEventStorageImpl *storage_impl);


#ifdef __cplusplus
}
#endif
