//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements conveninece API for posting a single chunk of Ticos data

#include "ticos/http/http_client.h"

#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/data_packetizer.h"

int ticos_http_client_post_chunk(void) {
  // A pre-flight check before we attempt to setup an HTTP client
  // If there's no data to send, just early return
  bool more_data = ticos_packetizer_data_available();
  if (!more_data) {
    // no new data to post
    return kTcsPostDataStatus_NoDataFound;
  }

  sTcsHttpClient *http_client = ticos_http_client_create();
  if (!http_client) {
    TICOS_LOG_ERROR("Failed to create HTTP client");
    return TicosInternalReturnCode_Error;
  }

  const int rv = ticos_http_client_post_data(http_client);
  if ((eTcsPostDataStatus)rv !=  kTcsPostDataStatus_Success) {
    TICOS_LOG_ERROR("Failed to post chunk: rv=%d", rv);
  }
  const uint32_t timeout_ms = 30 * 1000;
  ticos_http_client_wait_until_requests_completed(http_client, timeout_ms);
  ticos_http_client_destroy(http_client);
  return rv;
}
