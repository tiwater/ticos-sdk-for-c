//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Default implementation for buffer allocation while POSTing ticos chunk data

#include "ticos/esp_port/http_client.h"

#include <stdlib.h>
#include <stdint.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"

#ifndef TICOS_HTTP_CLIENT_MAX_BUFFER_SIZE
#  define TICOS_HTTP_CLIENT_MAX_BUFFER_SIZE  (256 * 1024)
#endif

#if TICOS_HTTP_CLIENT_MAX_BUFFER_SIZE < TICOS_HTTP_CLIENT_MIN_BUFFER_SIZE
#error "TICOS_HTTP_CLIENT_MAX_BUFFER_SIZE must be greater than 1024 bytes"
#endif

TICOS_WEAK
void *ticos_http_client_allocate_chunk_buffer(size_t *buffer_size) {
  // The more data we can pack into one http request, the more efficient things will
  // be from a network perspective. Let's start by trying to use a 16kB buffer and slim
  // things down if there isn't that much space available
  size_t try_alloc_size = TICOS_HTTP_CLIENT_MAX_BUFFER_SIZE;
  const uint32_t min_alloc_size = TICOS_HTTP_CLIENT_MIN_BUFFER_SIZE;

  void *buffer = NULL;
  while (try_alloc_size > min_alloc_size) {
    buffer = malloc(try_alloc_size);
    if (buffer != NULL) {
      *buffer_size = try_alloc_size;
      break;
    }
    try_alloc_size /= 2;
  }

  return buffer;
}

TICOS_WEAK
void ticos_http_client_release_chunk_buffer(void *buffer) {
  free(buffer);
}
