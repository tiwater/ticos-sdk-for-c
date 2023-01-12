//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Reference implementation of platform dependencies for the Ticos HTTP Client when using the
//! WICED SDK

#include "ticos/http/platform/http_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/math.h"
#include "ticos/panics/assert.h"
#include "ticos_platform_wiced.h"

#include "http.h"
#include "http_client.h"
#include "wiced_management.h"
#include "wiced_tls.h"
#include "wwd_debug.h"

#define TICOS_DNS_TIMEOUT_MS (10000)
#define TICOS_HTTP_CONNECT_TIMEOUT_MS (10000)
#define TICOS_HTTP_POST_DATA_READ_BUFFER_SIZE (256)
#define TICOS_HTTP_MIME_TYPE_BINARY "application/octet-stream"
// API expects colon+space in fieldname :/
#define TICOS_HTTP_PROJECT_KEY_HEADER_WICED TICOS_HTTP_PROJECT_KEY_HEADER ": "

typedef struct TcsHttpClient {
  http_client_t client;
  http_client_configuration_info_t config;
  bool is_dns_lookup_done;
  wiced_ip_address_t ip_address;

  bool is_request_pending;
  bool request_error_occurred;
  http_request_t request;
  TicosHttpClientResponseCallback callback;

  void *callback_ctx;
} sTcsHttpClient;

int ticos_platform_http_response_get_status(const sTcsHttpResponse *response, uint32_t *status_out) {
  TICOS_ASSERT(response);
  http_response_t *wiced_response = (http_response_t *)response;
  http_status_line_t status_line = {0};
  if (WICED_SUCCESS != http_get_status_line(
      wiced_response->response_hdr, wiced_response->response_hdr_length, &status_line)) {
    return -1;
  }
  if (status_out) {
    *status_out = status_line.code;
  }
  return 0;
}

static void prv_finalize_request_and_run_callback(sTcsHttpClient *client, http_response_t *response) {
  if (!client->is_request_pending) {
    return;
  }
  if (client->callback) {
    client->callback((const sTcsHttpResponse *) response, client->callback_ctx);
  }

  uint32_t http_status = 0;
  const int rv = ticos_platform_http_response_get_status((const sTcsHttpResponse *)response,
                                                            &http_status);
  if (rv != 0) {
    TICOS_LOG_ERROR("Request failed. No HTTP status: %d", rv);
    return;
  }
  client->request_error_occurred = (http_status < 200) || (http_status >= 300);
  client->callback = NULL;
  client->callback_ctx = NULL;
  http_request_deinit(&client->request);
  client->is_request_pending = false;
}

static void prv_handle_data_received(sTcsHttpClient *client, http_response_t *response) {
  if (response->request != &client->request) {
    TICOS_LOG_DEBUG("Recv data for different req");
    return;
  }
  if (response->response_hdr == NULL) {
    // Will it come later?
    TICOS_LOG_DEBUG("NULL header");
    return;
  }
  http_status_line_t status_line;
  if (WICED_SUCCESS != http_get_status_line(response->response_hdr, response->response_hdr_length, &status_line)) {
    TICOS_LOG_DEBUG("Couldn't parse status line");
    return;
  }
  prv_finalize_request_and_run_callback(client, response);
}

static void prv_http_event_handler(http_client_t *wiced_client, http_event_t event, http_response_t *response) {
  TICOS_STATIC_ASSERT(offsetof(sTcsHttpClient, client) == 0, "Expecting first member to be http_client_t client");
  sTcsHttpClient *client = (sTcsHttpClient *)wiced_client;

  switch (event) {
    case HTTP_CONNECTED:
      break;

    case HTTP_DISCONNECTED: {
      prv_finalize_request_and_run_callback(client, NULL);
      break;
    }

    case HTTP_DATA_RECEIVED: {
      prv_handle_data_received(client, response);
      break;
    }

    default:
      break;
  }
}

sTcsHttpClient *ticos_platform_http_client_create(void) {
  if (wiced_network_is_up(WICED_STA_INTERFACE) == WICED_FALSE) {
    // If the network is not up (WiFi is not joined) http_client_init() will trip an assert... :/ Race prone?
    goto error;
  }
  sTcsHttpClient *client = malloc(sizeof(sTcsHttpClient));
  if (!client) {
    goto error;
  }
  memset(client, 0, sizeof(*client));

  if (WICED_SUCCESS != http_client_init(&client->client, WICED_STA_INTERFACE, prv_http_event_handler, NULL)) {
    goto error;
  }

  const char *hostname = TICOS_HTTP_GET_CHUNKS_API_HOST();
  client->config = (http_client_configuration_info_t) {
      .flag = (http_client_configuration_flags_t)(
          HTTP_CLIENT_CONFIG_FLAG_SERVER_NAME | HTTP_CLIENT_CONFIG_FLAG_MAX_FRAGMENT_LEN),
      .server_name = (uint8_t *)hostname,
      .max_fragment_length = TLS_FRAGMENT_LENGTH_1024,
  };
  if (WICED_SUCCESS != http_client_configure(&client->client, &client->config)) {
    http_client_deinit(&client->client);
    goto error;
  }

  /* if you set hostname, library will make sure subject name in the server certificate is matching with host name
   * you are trying to connect. pass NULL if you don't want to enable this check */
  client->client.peer_cn = (uint8_t *)hostname;

  return client;

error:
  free(client);
  return NULL;
}

static bool prv_do_dns_lookup(sTcsHttpClient *client) {
  if (client->is_dns_lookup_done) {
    return true;
  }
  const wiced_result_t dns_rv = wiced_hostname_lookup((const char *)client->config.server_name, &client->ip_address,
      TICOS_DNS_TIMEOUT_MS, WICED_STA_INTERFACE);
  if (WICED_SUCCESS != dns_rv) {
    TICOS_LOG_ERROR("DNS lookup failed: %d", dns_rv);
    return false;
  }
  client->is_dns_lookup_done = true;
  return true;
}

static wiced_result_t prv_send_chunk_in_http_request(
    sTcsHttpClient *client, const char *url,
    TicosHttpClientResponseCallback callback, void *ctx) {
  uint8_t *buffer = malloc(TICOS_HTTP_POST_DATA_READ_BUFFER_SIZE);
  if (!buffer) {
    TICOS_LOG_ERROR("%s: malloc fail", __func__);
    return WICED_ERROR;
  }

  sPacketizerMetadata metadata;
  const sPacketizerConfig cfg = {
    // We will send an entire ticos data message in a single http request
    .enable_multi_packet_chunk = true,
  };
  bool md = ticos_packetizer_begin(&cfg, &metadata);
  if (!md) {
    // we should only be in this function when there _is_ data available
    goto error;
  }

  char content_length[12] = {0};
  snprintf(content_length, sizeof(content_length), "%d",
           (int)metadata.single_chunk_message_length);

  const http_header_field_t headers[] = {
      [0] = {
          .field = HTTP_HEADER_HOST,
          .field_length = sizeof(HTTP_HEADER_HOST) - 1,
          .value = (char *)client->config.server_name,
          .value_length = strlen((const char *)client->config.server_name),
      },
      [1] = {
          .field = TICOS_HTTP_PROJECT_KEY_HEADER_WICED,
          .field_length = sizeof(TICOS_HTTP_PROJECT_KEY_HEADER_WICED) - 1,
          .value = (char *)g_tcs_http_client_config.api_key,
          .value_length = strlen((const char *)g_tcs_http_client_config.api_key),
      },
      [2] = {
          .field = HTTP_HEADER_CONTENT_LENGTH,
          .field_length = sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1,
          .value = content_length,
          .value_length = strlen(content_length),
      },
      [3] = {
          .field = HTTP_HEADER_CONTENT_TYPE,
          .field_length = sizeof(HTTP_HEADER_CONTENT_TYPE) - 1,
          .value = TICOS_HTTP_MIME_TYPE_BINARY,
          .value_length = sizeof(TICOS_HTTP_MIME_TYPE_BINARY) - 1,
      }
  };

  http_request_t *const request = &client->request;

  WICED_VERIFY(http_request_init(request, &client->client, HTTP_POST, url, HTTP_1_1));
  WICED_VERIFY(http_request_write_header(request, headers, TICOS_ARRAY_SIZE(headers)));
  WICED_VERIFY(http_request_write_end_header(request));

  eTicosPacketizerStatus packetizer_status;
  do {
    size_t buffer_len = TICOS_HTTP_POST_DATA_READ_BUFFER_SIZE;
    packetizer_status =
        ticos_packetizer_get_next(buffer, &buffer_len);
    if (packetizer_status == kTicosPacketizerStatus_NoMoreData) {
      break;
    }

    const wiced_result_t write_rv = http_request_write(request, buffer, buffer_len);
    if (WICED_SUCCESS != write_rv) {
      TICOS_LOG_ERROR("http_request_write failed: %u", write_rv);
      goto error;
    }
  } while (packetizer_status != kTicosPacketizerStatus_EndOfChunk);

  // we have finished assembling one chunk so flush the request
  const wiced_result_t flush_rv = http_request_flush(request);
  if (WICED_SUCCESS != flush_rv) {
    TICOS_LOG_ERROR("http_request_flush failed: %u", flush_rv);
    goto error;
  }

  free(buffer);
  client->callback = callback;
  client->callback_ctx = ctx;
  client->is_request_pending = true;
  return WICED_SUCCESS;

error:
  http_request_deinit(&client->request);
  free(buffer);
  return WICED_ERROR;
}

// return different error codes from each exit point so it's easier to determine what went wrong
typedef enum {
  kTicosPlatformHttpPost_AlreadyPending = -1,
  kTicosPlatformHttpPost_GetSpiAddressFailed = -2,
  kTicosPlatformHttpPost_DnsLookupFailed = -3,
  kTicosPlatformHttpPost_BuildUrlFailed = -4,
  kTicosPlatformHttpPost_HttpRequestFailure = -5,
} eTicosPlatformHttpPost;

int ticos_platform_http_client_post_data(
    sTcsHttpClient *client, TicosHttpClientResponseCallback callback, void *ctx) {
  if (client->is_request_pending) {
    TICOS_LOG_ERROR("Data post request already pending!");
    return kTicosPlatformHttpPost_AlreadyPending;
  }

  // Early exit if there is no new data to send
  if (!ticos_packetizer_data_available()) {
    return kTcsPostDataStatus_NoDataFound;
  }

  if (!prv_do_dns_lookup(client)) {
    return kTicosPlatformHttpPost_DnsLookupFailed;
  }

  char url_buffer[TICOS_HTTP_URL_BUFFER_SIZE];
  if (!ticos_http_build_url(url_buffer, TICOS_HTTP_CHUNKS_API_SUBPATH)) {
    return kTicosPlatformHttpPost_BuildUrlFailed;
  }
  TICOS_LOG_DEBUG("Posting data to %s", url_buffer);

  const http_security_t security = g_tcs_http_client_config.disable_tls ? HTTP_NO_SECURITY : HTTP_USE_TLS;
  WICED_VERIFY(http_client_connect(&client->client, &client->ip_address, TICOS_HTTP_GET_CHUNKS_API_PORT(),
                                   security, TICOS_HTTP_CONNECT_TIMEOUT_MS));

  // Drain all the data that is available to be sent
  while (ticos_packetizer_data_available()) {
    int rv = prv_send_chunk_in_http_request(client, url_buffer, callback, ctx);
    if (rv != WICED_SUCCESS) {
      return TICOS_PLATFORM_SPECIFIC_ERROR(rv);
    }

    // Wait for the in-flight http request to complete
    ticos_platform_http_client_wait_until_requests_completed(
        client, TICOS_HTTP_CONNECT_TIMEOUT_MS);

    if (client->request_error_occurred) {
      TICOS_LOG_ERROR("Terminating data transfer because error occurred");
      ticos_packetizer_abort();
      return kTicosPlatformHttpPost_HttpRequestFailure;
    }
  }

  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(
    sTcsHttpClient *client, uint32_t timeout_ms) {
  uint32_t waited_ms = 0;
  while (client->is_request_pending) {
    // Could also be implemented using a semaphore
    wiced_rtos_delay_milliseconds(100);
    waited_ms += 100;
    if (waited_ms >= timeout_ms) {
      return -1;
    }
  }
  return 0;
}

int ticos_platform_http_client_destroy(sTcsHttpClient *client) {
  http_client_deinit((http_client_t *)client);
  free(client);
  return 0;
}
