//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Ticos HTTP Client implementation which can be used to send data to the Ticos cloud for
//! processing

#include "ticos/http/http_client.h"

#include <stdio.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/http/platform/http_client.h"

bool ticos_http_build_url(char url_buffer[TICOS_HTTP_URL_BUFFER_SIZE], const char *subpath) {
  sTicosDeviceInfo device_info;
  ticos_platform_get_device_info(&device_info);


  const int rv = snprintf(url_buffer, TICOS_HTTP_URL_BUFFER_SIZE, "%s://%s" TICOS_HTTP_CHUNKS_API_PREFIX "%s/%s",
                          TICOS_HTTP_GET_SCHEME(), TICOS_HTTP_GET_CHUNKS_API_HOST(), subpath, device_info.device_serial);
  return (rv < TICOS_HTTP_URL_BUFFER_SIZE);
}

sTcsHttpClient *ticos_http_client_create(void) {
  return ticos_platform_http_client_create();
}

static void prv_handle_post_data_response(const sTcsHttpResponse *response, TICOS_UNUSED void *ctx) {
  if (!response) {
    return;  // Request failed
  }
  uint32_t http_status = 0;
  const int rv = ticos_platform_http_response_get_status(response, &http_status);
  if (rv != 0) {
    TICOS_LOG_ERROR("Request failed. No HTTP status: %d", rv);
    return;
  }
  if (http_status < 200 || http_status >= 300) {
    // Redirections are expected to be handled by the platform implementation
    TICOS_LOG_ERROR("Request failed. HTTP Status: %"PRIu32, http_status);
    return;
  }
}

int ticos_http_client_post_data(sTcsHttpClient *client) {
  if (!client) {
    return TicosInternalReturnCode_InvalidInput;
  }
  return ticos_platform_http_client_post_data(client, prv_handle_post_data_response, NULL);
}

int ticos_http_client_wait_until_requests_completed(sTcsHttpClient *client, uint32_t timeout_ms) {
  if (!client) {
    return TicosInternalReturnCode_InvalidInput;
  }
  return ticos_platform_http_client_wait_until_requests_completed(client, timeout_ms);
}

int ticos_http_client_destroy(sTcsHttpClient *client) {
  if (!client) {
    return TicosInternalReturnCode_InvalidInput;
  }
  return ticos_platform_http_client_destroy(client);
}
