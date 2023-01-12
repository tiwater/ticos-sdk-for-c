#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A backend which serializes logs that have been compressed using
//! "ticos/core/compact_log_helpers.h" into an array of CBOR encoded arguments.
//! This enables logs to be easily inserted into Ticos events or base64/hexdump'd out
//! over the CLI.

#include <stdbool.h>

#include "ticos/util/cbor.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Serializes a "compact" log into a CBOR array representation
//!
//! @param encoder The cbor encoder to write into
//! @param log_id A unique identifier for the log being serialized
//! @param compressed_fmt The formatters for the log. See Tcs_GET_COMPRESSED_LOG_FMT in
//!  "ticos/core/compact_log_helpers.h" for more context
//! @param args The list of arguments to encode
//!
//! @return true if serialization succeeded, false otherwise
bool ticos_vlog_compact_serialize(sTicosCborEncoder *encoder, uint32_t log_id,
    uint32_t compressed_fmt, va_list args);

//! Same as "ticos_vlog_compact_serialize" but takes a variable list of arguments
bool ticos_log_compact_serialize(sTicosCborEncoder *encoder, uint32_t log_id,
    uint32_t compressed_fmt,  ...);

#ifdef __cplusplus
}
#endif
