//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

#include "ticos/core/data_export.h"

#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/sdk_assert.h"
#include "ticos/util/base64.h"

TICOS_WEAK
void ticos_data_export_base64_encoded_chunk(const char *base64_chunk) {
  TICOS_LOG_INFO("%s", base64_chunk);
}

static void prv_ticos_data_export_chunk(void *chunk_data, size_t chunk_data_len) {
  TICOS_SDK_ASSERT(chunk_data_len <= TICOS_DATA_EXPORT_CHUNK_MAX_LEN);

  char base64[TICOS_DATA_EXPORT_BASE64_CHUNK_MAX_LEN];

  memcpy(base64, TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX,
         TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX_LEN);

  size_t write_offset = TICOS_DATA_EXPORT_BASE64_CHUNK_PREFIX_LEN;

  ticos_base64_encode(chunk_data, chunk_data_len, &base64[write_offset]);
  write_offset += TICOS_BASE64_ENCODE_LEN(chunk_data_len);

  memcpy(&base64[write_offset], TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX,
         TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX_LEN);
  write_offset += TICOS_DATA_EXPORT_BASE64_CHUNK_SUFFIX_LEN;

  base64[write_offset] = '\0';

  ticos_data_export_base64_encoded_chunk(base64);
}

//! Note: We disable optimizations for this function to guarantee the symbol is
//! always exposed and our GDB test script (https://ticos.io/send-chunks-via-gdb)
//! can be installed to watch and post chunks every time it is called.
TICOS_NO_OPT
void ticos_data_export_chunk(void *chunk_data, size_t chunk_data_len) {
  prv_ticos_data_export_chunk(chunk_data, chunk_data_len);
}

static bool prv_try_send_ticos_data(void) {
  // buffer to copy chunk data into
  uint8_t buf[TICOS_DATA_EXPORT_CHUNK_MAX_LEN];
  size_t buf_len = sizeof(buf);
  bool data_available = ticos_packetizer_get_chunk(buf, &buf_len);
  if (!data_available ) {
    return false; // no more data to send
  }
  // send payload collected to chunks/ endpoint
  ticos_data_export_chunk(buf, buf_len);
  return true;
}

void ticos_data_export_dump_chunks(void) {
  while (prv_try_send_ticos_data()) { }
}
