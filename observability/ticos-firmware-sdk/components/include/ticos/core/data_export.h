#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//!
//! Utilities for exporting data collected by the Ticos SDK ("chunks") to a file or a log
//! stream for upload to the Ticos cloud.
//!
//! This can be used for production use-cases where data is extracted over pre-existing logging
//! facilities or during initial bringup before another transport is in place.
//!
//! The extracted data can be published to the Ticos cloud using the ticos-cli:
//! $ ticos --project-key ${YOUR_PROJECT_KEY} post-chunk --encoding sdk_data_export your_exported_data.txt
//!
//! A step-by-step integration guide with more details can be found at:
//!   https://ticos.io/chunk-data-export

#include <stddef.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/util/base64.h"

#ifdef __cplusplus
extern "C" {
#endif

//! A utility for dumping all the currently collected Ticos Data
//!
//! While there is still data available to send, this API makes calls to
//! 'ticos_data_export_chunk()'
void ticos_data_export_dump_chunks(void);

//! Called by 'ticos_data_export_chunk' once a chunk has been formatted as a string
//!
//! @note Defined as a weak function so an end user can override it and control where the chunk is
//! written. The default implementation prints the "chunk" by calling TICOS_LOG_INFO() but an
//! end user could override to save chunks elsewhere (for example a file).
//!
//! @param chunk_str - A NUL terminated string with a base64 encoded Ticos "chunk" with a "MC:"
//!   as a header and ":" as a footer.
void ticos_data_export_base64_encoded_chunk(const char *chunk_str);

//! Encodes a Ticos "chunk" as a string and calls ticos_data_export_base64_encoded_chunk
//!
//! @note The string is formatted as 'MC:CHUNK_DATA_BASE64_ENCODED:'. We wrap the base64 encoded
//! chunk in a prefix ("MC:") and suffix (":") so the chunks can be even be extracted from logs with
//! other data
//!
//! @note This command can also be used with the Ticos GDB command "ticos
//! install_chunk_handler" to "drain" chunks up to the Ticos cloud directly from GDB. This can
//! be useful when working on integrations and initially getting a transport path in place:
//!
//!   (gdb) source $TICOS_FIRMWARE_SDK/scripts/ticos_gdb.py
//!   (gdb) ticos install_chunk_handler --help
//!   (gdb) ticos install_chunk_handler -pk <YOUR_PROJECT_KEY>
//!
//!   For more details see https://ticos.io/posting-chunks-with-gdb
//!
//! @param chunk_data The binary chunk data to send
//! @param chunk_data_len The length of the chunk data to send. Must be less than
//!  or equal to TICOS_DATA_EXPORT_CHUNK_MAX_LEN
void ticos_data_export_chunk(void *chunk_data, size_t chunk_data_len);

#define TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX "MC:" // *M*emfault *C*hunk
#define TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX_LEN \
  TICOS_STATIC_STRLEN(TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX)

#define TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX ":"
#define TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX_LEN \
  TICOS_STATIC_STRLEN(TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX)

#define TICOS_DATA_EXPORT_BASE64_CHUNK_MAX_LEN                       \
  (TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX_LEN +                       \
   TICOS_BASE64_ENCODE_LEN(TICOS_DATA_EXPORT_CHUNK_MAX_LEN) +     \
   TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX_LEN +                       \
   1 /* '\0' */ )

#ifdef __cplusplus
}
#endif
