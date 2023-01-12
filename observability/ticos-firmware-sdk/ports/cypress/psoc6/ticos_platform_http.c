//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Implements dependency functions required to utilize Ticos's http client
//! for posting data collected via HTTPS

#include <stdbool.h>

#include "ticos/components.h"

#include "cy_secure_sockets.h"
#include "cy_tls.h"

struct TcsHttpClient {
  bool active;
  cy_socket_t handle;
  cy_socket_sockaddr_t sock_addr;
};

typedef struct TcsHttpResponse {
  uint32_t status_code;
} sTcsHttpResponse;

static sTcsHttpClient s_client;

static cy_rslt_t prv_teardown_connection(sTcsHttpClient *client) {
  cy_rslt_t result = cy_socket_disconnect(client->handle, 0);
  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR("cy_socket_disconnect failed: rv=0x%x", (int)result);
  }

  cy_socket_delete(client->handle);
  client->active = false;
  return 0;
}

static cy_rslt_t prv_disconnect_handler(cy_socket_t socket_handle, void *arg) {
  TICOS_LOG_DEBUG("Ticos Socket Disconnected dropped");
  prv_teardown_connection(&s_client);
  return 0;
}

static cy_rslt_t prv_open_socket(sTcsHttpClient *client) {
  cy_rslt_t result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_STREAM,
                                      CY_SOCKET_IPPROTO_TLS, &client->handle);

  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR("cy_socket_create failed, rv=0x%x", (int)result);
    return result;
  }

  cy_socket_opt_callback_t disconnect_listener = {
      .callback = prv_disconnect_handler, .arg = NULL};

  result = cy_socket_setsockopt(
      client->handle, CY_SOCKET_SOL_SOCKET, CY_SOCKET_SO_DISCONNECT_CALLBACK,
      &disconnect_listener, sizeof(cy_socket_opt_callback_t));
  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR(
        "cy_socket_setsockopt CY_SOCKET_SO_DISCONNECT_CALLBACK failed, rv=0x%x", (int)result);
    return result;
  }

  cy_socket_tls_auth_mode_t tls_auth_mode = CY_SOCKET_TLS_VERIFY_REQUIRED;
  result = cy_socket_setsockopt(client->handle, CY_SOCKET_SOL_TLS,
                                CY_SOCKET_SO_TLS_AUTH_MODE, &tls_auth_mode,
                                sizeof(cy_socket_tls_auth_mode_t));
  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR("cy_socket_setsockopt CY_SOCKET_SO_TLS_AUTH_MODE failed, rv=0x%x",
                       (int)result);
  }

  result = cy_socket_connect(client->handle, &client->sock_addr,
                             sizeof(cy_socket_sockaddr_t));
  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR("cy_socket_connect failed, 0x%x", (int)result);
  }

  return result;
}

sTcsHttpClient *ticos_platform_http_client_create(void) {
  if (s_client.active) {
    TICOS_LOG_ERROR("Ticos HTTP client already in use");
    return NULL;
  }

  s_client = (sTcsHttpClient) {
    .sock_addr = {
      .port = TICOS_HTTP_GET_CHUNKS_API_PORT(),
    },
  };

  cy_rslt_t result = cy_socket_gethostbyname(
      TICOS_HTTP_CHUNKS_API_HOST, CY_SOCKET_IP_VER_V4, &s_client.sock_addr.ip_address);
  if (result != CY_RSLT_SUCCESS) {
    TICOS_LOG_ERROR("DNS lookup failed: 0x%x", (int)result);
    return NULL;
  }

  result = prv_open_socket(&s_client);
  if (result != CY_RSLT_SUCCESS) {
    return NULL;
  }

  s_client.active = true;
  return &s_client;
}


static bool prv_try_send(sTcsHttpClient *client, const uint8_t *buf, size_t buf_len) {
  cy_rslt_t idx = 0;
  while (idx != buf_len) {
    uint32_t bytes_sent = 0;
    int rv = cy_socket_send(client->handle, &buf[idx], buf_len - idx, CY_SOCKET_FLAGS_NONE, &bytes_sent);
    if (rv == CY_RSLT_SUCCESS && bytes_sent > 0) {
      idx += bytes_sent;
      continue;
    }

    TICOS_LOG_ERROR("Data Send Error: bytes_sent=%d, cy_rslt=0x%x", (int)bytes_sent, rv);
    return false;

  }
  return true;
}

static bool prv_send_data(const void *data, size_t data_len, void *ctx) {
  sTcsHttpClient *client = (sTcsHttpClient *)ctx;
  return prv_try_send(client, data, data_len);
}

static bool prv_read_socket_data(sTcsHttpClient *client, void *buf, size_t *buf_len) {
  uint32_t buf_len_out;
  cy_rslt_t result = cy_socket_recv(client->handle, buf, *buf_len, CY_SOCKET_FLAGS_NONE, &buf_len_out);
  *buf_len = (size_t)buf_len_out;
  return result == CY_RSLT_SUCCESS;
}

int ticos_platform_http_response_get_status(const sTcsHttpResponse *response, uint32_t *status_out) {
  TICOS_SDK_ASSERT(response != NULL);

  *status_out = response->status_code;
  return 0;
}

static int prv_wait_for_http_response(sTcsHttpClient *client) {
  sTicosHttpResponseContext ctx = { 0 };
  while (1) {
    // We don't expect any response that needs to be parsed so
    // just use an arbitrarily small receive buffer
    char buf[32];
    size_t bytes_read = sizeof(buf);
    if (!prv_read_socket_data(client, buf, &bytes_read)) {
      return -1;
    }

    bool done = ticos_http_parse_response(&ctx, buf, bytes_read);
    if (done) {
      TICOS_LOG_DEBUG("Response Complete: Parse Status %d HTTP Status %d!",
                         (int)ctx.parse_error, ctx.http_status_code);
      TICOS_LOG_DEBUG("Body: %s", ctx.http_body);
      return ctx.http_status_code;
    }
  }
}

int ticos_platform_http_client_post_data(
    sTcsHttpClient *client, TicosHttpClientResponseCallback callback, void *ctx) {
  if (!client->active) {
    return -1;
  }

  const sPacketizerConfig cfg = {
    // let a single msg span many "ticos_packetizer_get_next" calls
    .enable_multi_packet_chunk = true,
  };

  // will be populated with size of entire message queued for sending
  sPacketizerMetadata metadata;
  const bool data_available = ticos_packetizer_begin(&cfg, &metadata);
  if (!data_available) {
    TICOS_LOG_DEBUG("No more data to send");
    return kTcsPostDataStatus_NoDataFound;
  }

  ticos_http_start_chunk_post(prv_send_data, client, metadata.single_chunk_message_length);

  // Drain all the data that is available to be sent
  while (1) {
    // Arbitrarily sized send buffer.
    uint8_t buf[128];
    size_t buf_len = sizeof(buf);
    eTicosPacketizerStatus status = ticos_packetizer_get_next(buf, &buf_len);
    if (status == kTicosPacketizerStatus_NoMoreData) {
      break;
    }

    if (!prv_try_send(client, buf, buf_len)) {
      // unexpected failure, abort in-flight transaction
      ticos_packetizer_abort();
      return false;
    }

    if (status == kTicosPacketizerStatus_EndOfChunk) {
      break;
    }
  }

  // we've sent a chunk, drain status
  sTcsHttpResponse response = {
    .status_code = prv_wait_for_http_response(client),
  };

  if (callback) {
    callback(&response, ctx);
  }

  return 0;
}

int ticos_platform_http_client_destroy(sTcsHttpClient *client) {
  if (!client->active) {
    return -1;
  }

  prv_teardown_connection(client);
  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(
    sTcsHttpClient *client, uint32_t timeout_ms) {
  // No-op because ticos_platform_http_client_post_data() is synchronous
  return 0;
}
