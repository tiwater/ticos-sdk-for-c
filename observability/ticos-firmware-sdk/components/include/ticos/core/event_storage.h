#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Stores serialized event information that is ready to be sent up to the Ticos cloud (i.e
//! Heartbeat Metrics, Reboot Reasons & Trace events). Must be initialized on system boot.
//!
//! @note If calls to data_packetizer.c are made on a different task than the one
//! TicosPlatformTimerCallback is invoked on, ticos_lock() & ticos_unlock() should also
//! be implemented by the platform
//!
//! @note Recorded events are always written into RAM for minimal latency. Users of the API can
//! (optionally) implement the non-volatile event storage platform API and periodically flush
//! events to a non-volatile storage medium. More details can be found in
//! "ticos/core/platform/nonvolatile_event_storage.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The unity test framework (http://www.throwtheswitch.org/unity) fails to generate mocks when
// opaque pointers are used in a header.  To work around, the problem, we pull in the full internal
// definition for "sTicosEventStorageImpl" when the unity framework is being used.
#if defined(UNITY_INCLUDE_CONFIG_H)
#include "ticos/core/event_storage_implementation.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosEventStorageImpl sTicosEventStorageImpl;

//! Must be called by the customer on boot to setup event storage.
//!
//! This is where serialized event data like heartbeat metrics, reboot tracking info, and
//! trace event data is stored as it waits to be drained (via call's
//! to ticos_packetizer_get_next()).
//!
//! For any module using the event store the worst case size needed can be computed using the
//! exported APIs:
//!   ticos_reboot_tracking_compute_worst_case_storage_size()
//!   ticos_metrics_heartbeat_compute_worst_case_storage_size()
//!   ticos_trace_event_compute_worst_case_storage_size()
//!
//! For example, if a device connects to the Internet once/hour and collects a heartbeat at this
//! interval as well, the buffer size to allocate can easily be computed as:
//!  buf_len = ticos_metrics_heartbeat_compute_worst_case_storage_size() * 1
//!
//! When a module using the event store is initialized a WARNING will print on boot and an error
//! code will return if it is not appropriately sized to hold at least one event.
//!
//! @param buf The buffer to use for event storage
//! @param buf_len The length of the buffer to use for event storage
//!
//! @return a handle to the event storage implementation on success & false on failure.
//!  This handle will need to be provided to modules which use the event store on initialization
const sTicosEventStorageImpl *ticos_events_storage_boot(void *buf, size_t buf_len);

typedef struct TicosEventStorageInfo {
  size_t bytes_used;
  size_t bytes_free;
} sTicosEventStorageInfo;

typedef struct TicosEventStoragePersistCbStatus {
  //! Summarizes the utilization of the RAM buffer passed in ticos_events_storage_boot()
  sTicosEventStorageInfo volatile_storage;
} sTicosEventStoragePersistCbStatus;

//! Invoked to request that events be saved to non-volatile storage
//!
//! @param status Summarizes the current state of event storage. The user of the SDK
//!  can optionally use this information to decide whether or not to persist events to
//!  non-volatile storage.
//!
//! @note By default this is a weak function which behaves as a no-op.
//!
//! @note The intended use of the API is to provide platforms which implement non-volatile event
//! storage a "hook" to help them determine when to flush events from volatile to non-volatile
//! storage. This function is invoked each time an event:
//!  - is saved into the RAM-backed buffer
//!  - is consumed from non-volatile storage _and_ there are still events in
//!    volatile storage. (i.e If non-volatile storage is full, reading events will free up space
//!    and then events residing in RAM can be persisted into the new freed up space)
//!
//! @note It is safe to call "ticos_event_storage_persist()" both synchronously and
//!  asynchronously from this callback
void ticos_event_storage_request_persist_callback(
    const sTicosEventStoragePersistCbStatus *status);

//! Saves events which have been collected into non-volatile storage
//!
//! @return number of events saved or <0 for unexpected errors
int ticos_event_storage_persist(void);

//! Simple API call to retrieve the number of bytes used in the allocated event storage buffer.
//! Returns zero if the storage has not been allocated.
size_t ticos_event_storage_bytes_used(void);

//! Simple API call to retrieve the number of bytes free (unused) in the allocated event storage buffer.
//! Returns zero if the storage has not been allocated.
size_t ticos_event_storage_bytes_free(void);

#ifdef __cplusplus
}
#endif
