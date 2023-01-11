//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake implementation of the http client platform API

#include "ticos/http/platform/http_client.h"

#include "ticos/core/compiler.h"

int ticos_platform_http_response_get_status(
    TICOS_UNUSED const sMfltHttpResponse *response, uint32_t *status_out) {
  if (status_out) {
    *status_out = 200;
  }
  return 0;
}

static sMfltHttpClient *s_client = (sMfltHttpClient *)~0;

sMfltHttpClient *ticos_platform_http_client_create(void) {
  return s_client;
}

int ticos_platform_http_client_post_data(
    TICOS_UNUSED sMfltHttpClient *client,
    TICOS_UNUSED TicosHttpClientResponseCallback callback,
    TICOS_UNUSED void *ctx) {
  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(
    TICOS_UNUSED sMfltHttpClient *client,
    TICOS_UNUSED uint32_t timeout_ms) {
  return 0;
}

int ticos_platform_http_client_destroy(TICOS_UNUSED sMfltHttpClient *client) {
  return 0;
}
