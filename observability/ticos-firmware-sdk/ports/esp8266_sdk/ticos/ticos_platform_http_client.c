//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Wraps the ESP8266 ESP HTTP Client to facilitate posting
//! data from Ticos to cloud

#include "ticos/http/platform/http_client.h"

#include <string.h>

#include "esp_http_client.h"

#include "ticos/core/data_packetizer.h"

#include "ticos/config.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/debug_log.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/esp8266_port/core.h"
#include "ticos/esp8266_port/http_client.h"
#include "ticos/http/http_client.h"
#include "ticos/http/root_certs.h"
#include "ticos/http/utils.h"
#include "ticos/panics/assert.h"

#include "esp_wifi.h"

TICOS_WEAK
bool ticos_esp_port_data_available(void) {
  return ticos_packetizer_data_available();
}

TICOS_WEAK
bool ticos_esp_port_get_chunk(void *buf, size_t *buf_len) {
  return ticos_packetizer_get_chunk(buf, buf_len);
}

#ifndef TICOS_HTTP_DEBUG
#  define TICOS_HTTP_DEBUG (0)
#endif

#if TICOS_HTTP_DEBUG
static esp_err_t prv_http_event_handler(esp_http_client_event_t *evt) {
  switch(evt->event_id) {
    case HTTP_EVENT_ERROR:
      ticos_platform_log(kTicosPlatformLogLevel_Error, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ticos_platform_log(kTicosPlatformLogLevel_Info, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ticos_platform_log(kTicosPlatformLogLevel_Info, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ticos_platform_log(kTicosPlatformLogLevel_Info,
          "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ticos_platform_log(kTicosPlatformLogLevel_Info,
          "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // ticos_platform_log(kTicosPlatformLogLevel_Info, "%.*s", evt->data_len, (char*)evt->data);
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      ticos_platform_log(kTicosPlatformLogLevel_Info, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      ticos_platform_log(kTicosPlatformLogLevel_Info, "HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}
#endif  // TICOS_HTTP_DEBUG



static int prv_post_chunks(esp_http_client_handle_t client, void *buffer, size_t buf_len) {
  // drain all the chunks we have
  while (1) {
    // NOTE: Ideally we would be able to enable multi packet chunking which would allow a chunk to
    // span multiple calls to ticos_packetizer_get_next(). Unfortunately the esp-idf does not
    // have a POST mechanism that can use a callback so our POST size is limited by the size of the
    // buffer we can allocate.
    size_t read_size = buf_len;

    const bool more_data = ticos_esp_port_get_chunk(buffer, &read_size);
    if (!more_data) {
      break;
    }

    esp_http_client_set_post_field(client, buffer, read_size);
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

    esp_err_t err = esp_http_client_perform(client);
    if (ESP_OK != err) {
      return TICOS_PLATFORM_SPECIFIC_ERROR(err);
    }
  }

  return 0;
}

static char s_tcs_base_url_buffer[TICOS_HTTP_URL_BUFFER_SIZE];

sTcsHttpClient *ticos_platform_http_client_create(void) {
  ticos_http_build_url(s_tcs_base_url_buffer, "");

  // Mbed TLS parses each cert and loads it into RAM. Since the ESP8266 has a limited amount of
  // RAM, invoking HTTP with multiple certs at once increases the chances of running out of
  // memory. Therefore, we issue a quick HTTP GET to determine the cert we should be using rather
  // than opening a client with all certs loaded

  const char *ticos_root_certs[] = {
    TICOS_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_G2,
    TICOS_ROOT_CERTS_DIGICERT_GLOBAL_ROOT_CA,
    TICOS_ROOT_CERTS_BALTIMORE_CYBERTRUST_ROOT,
    TICOS_ROOT_CERTS_AMAZON_ROOT_CA1
  };

  esp_http_client_handle_t client = NULL;

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(ticos_root_certs); i++) {
    const esp_http_client_config_t config = {
#if TICOS_HTTP_DEBUG
      .event_handler = prv_http_event_handler,
#endif
      .url = s_tcs_base_url_buffer,
      .cert_pem = g_tcs_http_client_config.disable_tls ? NULL :
      ticos_root_certs[i],
    };

    client = esp_http_client_init(&config);

    esp_http_client_set_header(client, TICOS_HTTP_PROJECT_KEY_HEADER, g_tcs_http_client_config.api_key);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
      return (sTcsHttpClient *)client;
    }
    TICOS_LOG_INFO("Retrying post with root CA %d", i + 1);
    esp_http_client_cleanup(client);
  }

  return NULL;
}

int ticos_platform_http_client_destroy(sTcsHttpClient *_client) {
  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  esp_err_t err = esp_http_client_cleanup(client);
  if (err == ESP_OK) {
    return 0;
  }

  return TICOS_PLATFORM_SPECIFIC_ERROR(err);
}

typedef struct TcsHttpResponse {
  uint16_t status;
} sTcsHttpResponse;

int ticos_platform_http_response_get_status(const sTcsHttpResponse *response, uint32_t *status_out) {
  TICOS_ASSERT(response);
  if (status_out) {
    *status_out = response->status;
  }
  return 0;
}

int ticos_platform_http_client_post_data(
    sTcsHttpClient *_client, TicosHttpClientResponseCallback callback, void *ctx) {
  if (!ticos_esp_port_data_available()) {
    return 0; // no new chunks to send
  }

  TICOS_LOG_DEBUG("Posting Ticos Data");

  size_t buffer_size = 0;
  void *buffer = ticos_http_client_allocate_chunk_buffer(&buffer_size);
  if ((buffer == NULL) || (buffer_size == 0)) {
    TICOS_LOG_ERROR("Unable to allocate POST buffer");
    return -1;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  char url[TICOS_HTTP_URL_BUFFER_SIZE];
  ticos_http_build_url(url, TICOS_HTTP_CHUNKS_API_SUBPATH);
  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Accept", "application/json");

  int rv = prv_post_chunks(client, buffer, buffer_size);
  ticos_http_client_release_chunk_buffer(buffer);
  if (rv != 0) {
    TICOS_LOG_ERROR("%s failed: %d", __func__, (int)rv);
    return rv;
  }

  const sTcsHttpResponse response = {
    .status = (uint32_t)esp_http_client_get_status_code(client),
  };
  if (callback) {
    callback(&response, ctx);
  }
  TICOS_LOG_DEBUG("Posting Ticos Data Complete!");
  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(
    sTcsHttpClient *client, uint32_t timeout_ms) {
  // No-op because ticos_platform_http_client_post_data() is synchronous
  return 0;
}

bool ticos_esp_port_wifi_connected(void) {
  wifi_ap_record_t ap_info;
  const bool connected = esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
  return connected;
}

int ticos_esp_port_http_client_post_data(void) {
  if (!ticos_esp_port_wifi_connected()) {
    TICOS_LOG_INFO("%s: Wifi unavailable", __func__);
    return -1;
  }

  if (!ticos_esp_port_data_available()) {
    return 0;
  }

  sTcsHttpClient *http_client = ticos_http_client_create();
  if (!http_client) {
    TICOS_LOG_ERROR("Failed to create HTTP client");
    return TicosInternalReturnCode_Error;
  }
  const eTcsPostDataStatus rv =
      (eTcsPostDataStatus)ticos_http_client_post_data(http_client);
  if (rv == kTcsPostDataStatus_NoDataFound) {
    TICOS_LOG_INFO("No new data found");
  } else {
    TICOS_LOG_INFO("Result: %d", (int)rv);
  }
  const uint32_t timeout_ms = 30 * 1000;
  ticos_http_client_wait_until_requests_completed(http_client, timeout_ms);
  ticos_http_client_destroy(http_client);
  return (int)rv;
}
