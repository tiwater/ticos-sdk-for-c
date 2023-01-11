#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Many embedded projects have custom data formats which can be useful to analyze when debugging
//! issue (i.e trace data, proprietary coredumps, logs). Ticos allows this arbitrary information
//! to be bundled as a "Custom Data Recording" (CDR). The following API provides a flexible way
//! for this type of data to be published to the Ticos cloud. Within the Ticos cloud,
//! recorded data can then easily be looked up by reason, time, and device.
//!
//! To start reporting custom data a user of the SDK must do the following:
//!
//!  1. Implement the sTicosCdrSourceImpl API for the data to be captured:
//!     static sTicosCdrSourceImpl s_my_custom_data_recording_source = {
//!         [... fill me in ...]
//!     }
//!  2. Register the implementation with this module by calling:
//!     ticos_cdr_register_source(&s_my_custom_data_recording_source)
//!
//! See documentation below for more tips on how to implement the API.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "ticos/core/platform/system_time.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_CDR_BINARY  "application/octet-stream"
#define TICOS_CDR_TEXT "text/plain"
#define TICOS_CDR_CSV "text/csv"

typedef struct TicosCdrMetadata {
  //! The time the recording was started:
  //!  .start_time = (sTicosCurrentTime) {
  //!     .type = kTicosCurrentTimeType_UnixEpochTimeSec,
  //!     .info = { .unix_timestamp_secs = ... },
  //!  }
  //!
  //! If no start_time is known, simply set the type to kTicosCurrentTimeType_Unknown
  //! and Ticos will use the time the data arrives on the server - duration_ms to approximate
  //! the start.
  sTicosCurrentTime start_time;

  //! The format of the CDR. Ticos uses this as a hint for visualization in the UI
  //!
  //! Typically this will be one of the TICOS_CDR_* defines from above:
  //!
  //! const char *mimetypes = { TICOS_CDR_BINARY };
  //! TicosCdrMetadata my_cdr = (TicosCdrMetadata) {
  //!   [...]
  //!   .mimetypes = mimetypes,
  //!   .num_mimetypes = TICOS_ARRAY_SIZE(mimetypes),
  //! }
  //!
  //! @note List is expected to be ordered from most specific to most generic description when
  //! multiple types are listed.
  const char **mimetypes;
  size_t num_mimetypes;

  //! The total size of the CDR data (in bytes)
  uint32_t data_size_bytes;

  //! The duration of time the recording tracks (can be 0 if unknown).
  uint32_t duration_ms; // optional

  //! The reason the data was captured (i.e "ble connection failure", "wifi stack crash")
  //!
  //! @note A project can have at most 100 unique reasons and the length of a given reason can not
  //! exceed 100 characters.
  const char *collection_reason;
} sTicosCdrMetadata;

typedef struct TicosCdrSourceImpl {
  //! Called to determine if a CDR is available
  //!
  //! @param metadata Metadata associated with recording. See comments within
  //! sTicosCdrMetadata typedef for more info.
  //!
  //! @note If a recording is available, metadata must be populated.
  //!
  //! @return true if a recording is available, false otherwise
  bool (*has_cdr_cb)(sTicosCdrMetadata *metadata);

  //! Called to read the data within a CDR.
  //!
  //! @note It is expected the data being returned here is from the CDR
  //! returned in the last "has_cdr_cb" call and that it is
  //! safe to call this function multiple times
  //!
  //! @param offset The offset within the CDR data to read
  //! @param data The buffer to populate with CDR data
  //! @param data_len The size of data to fill in the buffer
  //!
  //! return true if read was succesful and buf was entirely filled, false otherwise
  bool (*read_data_cb)(uint32_t offset, void *data, size_t data_len);

  //! Called once the recording has been processed
  //!
  //! At this point, the recording should be cleared and a subsequent call to has_cdr_cb
  //! should either return metadata for a new recording or false
  void (*mark_cdr_read_cb)(void);
} sTicosCdrSourceImpl;

//! Register a Custom Data Recording generator to publish data to the Ticos cloud
//!
//! @param impl The CDR source to register
//!
//! @note We recommend invoking this function at bootup and expect that the "impl" passed
//! is of static or global scope
//!
//! @return true if registration was succesful or false if the storage was full. If false is returned
//! TICOS_CDR_MAX_DATA_SOURCES needs to be increased.
bool ticos_cdr_register_source(const sTicosCdrSourceImpl *impl);

#ifdef __cplusplus
}
#endif
