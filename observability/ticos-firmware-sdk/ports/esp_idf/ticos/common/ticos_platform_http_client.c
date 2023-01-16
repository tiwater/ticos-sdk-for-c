//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Example implementation of platform dependencies on the ESP32 for the Ticos HTTP APIs

#include "ticos/config.h"

#if TICOS_ESP_HTTP_CLIENT_ENABLE

#include <string.h>

#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_wifi.h"
#include "ticos/components.h"
#include "ticos/esp_port/core.h"
#include "ticos/esp_port/http_client.h"
#include "ticos/esp_port/version.h"

#ifndef TICOS_HTTP_DEBUG
#define TICOS_HTTP_DEBUG (0)
#endif

//! Default buffer size for the URL-encoded device info parameters. This may
//! need to be set higher by the user if there are particularly long device info
//! strings
#ifndef TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN
#define TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN (48)
#endif

TICOS_STATIC_ASSERT(sizeof(CONFIG_TICOS_PROJECT_KEY) > 1,
                       "Ticos Project Key not configured. Please visit https://ticos.io/project-key "
                       "and add CONFIG_TICOS_PROJECT_KEY=\"YOUR_KEY\" to sdkconfig.defaults");
static char _buff[1024];
sTcsHttpClientConfig g_tcs_http_client_config = {
  .api_key = CONFIG_TICOS_PROJECT_KEY,
  // .chunks_api = { .host = "api.ticos.cn", .port = 80 },
  // .chunks_api = { .host = "api.dev.ticos.cc", .port = 80 },
  .disable_tls = true,
};

#if TICOS_HTTP_DEBUG
static esp_err_t prv_http_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
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
                            "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
                            evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ticos_platform_log(kTicosPlatformLogLevel_Info, "HTTP_EVENT_ON_DATA, len=%d",
                            evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // Write out data
        // ticos_platform_log(kTicosPlatformLogLevel_Info, "%.*s", evt->data_len,
        // (char*)evt->data);
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
  const esp_http_client_config_t config = {
#if TICOS_HTTP_DEBUG
    .event_handler = prv_http_event_handler,
#endif
    .url = s_tcs_base_url_buffer,
    .timeout_ms = CONFIG_TICOS_HTTP_CLIENT_TIMEOUT_MS,
    .cert_pem = g_tcs_http_client_config.disable_tls ? NULL : TICOS_ROOT_CERTS_PEM,
    .user_data = _buff,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_http_client_set_header(client, TICOS_HTTP_PROJECT_KEY_HEADER,
                             g_tcs_http_client_config.api_key);
  return (sTcsHttpClient *)client;
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

int ticos_platform_http_response_get_status(const sTcsHttpResponse *response,
                                               uint32_t *status_out) {
  TICOS_ASSERT(response);
  if (status_out) {
    *status_out = response->status;
  }
  return 0;
}

//! Check the 3 device info fields that aren't escaped. Return -1 if any need
//! escaping
static int prv_deviceinfo_needs_url_escaping(sTicosDeviceInfo *device_info) {
  const struct params_s {
    const char *name;
    const char *value;
  } params[] = {
    {
      .name = "device_serial",
      .value = device_info->device_serial,
    },
    {
      .name = "hardware_version",
      .value = device_info->hardware_version,
    },
    {
      .name = "software_type",
      .value = device_info->software_type,
    },
  };

  for (size_t i = 0; i < TICOS_ARRAY_SIZE(params); i++) {
    if (ticos_http_needs_escape(params[i].value, strlen(params[i].value))) {
      TICOS_LOG_ERROR(
        "OTA URL query param '%s' contains reserved characters and needs escaping: %s",
        params[i].name, params[i].value);
      return -1;
    }
  }

  return 0;
}

static int prv_build_latest_release_url(char *buf, size_t buf_len) {
  sTicosDeviceInfo device_info;
  ticos_platform_get_device_info(&device_info);

  // if any of the device info fields need escaping, abort
  if (prv_deviceinfo_needs_url_escaping(&device_info)) {
    return -1;
  }

  // URL encode software_version before appending it to the URL; most likely to
  // have reserved characters
  char urlencoded_software_version[TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN];
  int rv =
    ticos_http_urlencode(device_info.software_version, strlen(device_info.software_version),
                            urlencoded_software_version, sizeof(urlencoded_software_version));
  if (rv != 0) {
    TICOS_LOG_ERROR("Failed to URL encode software version");
    return -1;
  }

  return snprintf(buf, buf_len,
                  "%s://%s/api/v0/releases/latest/"
                  "url?device_serial=%s&hardware_version=%s&software_type=%s&current_version=%s",
                  TICOS_HTTP_GET_SCHEME(), TICOS_HTTP_GET_DEVICE_API_HOST(),
                  device_info.device_serial, device_info.hardware_version,
                  device_info.software_type, urlencoded_software_version);
}

static int prv_get_ota_update_url(char **download_url_out) {
  sTcsHttpClient *http_client = ticos_http_client_create();
  char *url = NULL;
  char *download_url = NULL;
  int status_code = -1;

  int rv = -1;
  if (http_client == NULL) {
    return rv;
  }

  // call once with no buffer to figure out space we need to allocate to hold url
  int url_len = prv_build_latest_release_url(NULL, 0);
  if (url_len < 0) {
    TICOS_LOG_ERROR("Failed to build OTA URL");
    goto cleanup;
  }

  const size_t url_buf_len = url_len + 1 /* for '\0' */;
  url = calloc(1, url_buf_len);
  if (url == NULL) {
    TICOS_LOG_ERROR("Unable to allocate url buffer (%dB)", (int)url_buf_len);
    goto cleanup;
  }

  if (prv_build_latest_release_url(url, url_buf_len) != url_len) {
    goto cleanup;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)http_client;

  // NB: For esp-idf versions > v3.3 will set the Host automatically as part
  // of esp_http_client_set_url() so this call isn't strictly necessary.
  //
  //  https://github.com/espressif/esp-idf/commit/a8755055467f3e6ab44dd802f0254ed0281059cc
  //  https://github.com/espressif/esp-idf/commit/d154723a840f04f3c216df576456830c884e7abd
  esp_http_client_set_header(client, "Host", TICOS_HTTP_GET_DEVICE_API_HOST());

  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_GET);
  // to keep the parsing simple, we will request the download url as plaintext
  esp_http_client_set_header(client, "Accept", "text/plain");

  const esp_err_t err = esp_http_client_open(client, 0);
  if (ESP_OK != err) {
    rv = TICOS_PLATFORM_SPECIFIC_ERROR(err);
    goto cleanup;
  }

  const int content_length = esp_http_client_fetch_headers(client);
  if (content_length < 0) {
    rv = TICOS_PLATFORM_SPECIFIC_ERROR(content_length);
    goto cleanup;
  }

  download_url = calloc(1, content_length + 1);
  if (download_url == NULL) {
    TICOS_LOG_ERROR("Unable to allocate download url buffer (%dB)", (int)download_url);
    goto cleanup;
  }

  int bytes_read = 0;
  while (bytes_read != content_length) {
    int len = esp_http_client_read(client, &download_url[bytes_read], content_length - bytes_read);
    if (len < 0) {
      rv = TICOS_PLATFORM_SPECIFIC_ERROR(len);
      goto cleanup;
    }
    bytes_read += len;
  }

  status_code = esp_http_client_get_status_code(client);
  if (status_code != 200 && status_code != 204) {
    TICOS_LOG_ERROR("OTA Query Failure. Status Code: %d", status_code);
    TICOS_LOG_INFO("Request Body: %s", download_url);
    goto cleanup;
  }

  // Lookup to see if a release is available was succesful!
  rv = 0;

cleanup:
  free(url);
  ticos_http_client_destroy(http_client);

  if (status_code == 200) {
    *download_url_out = download_url;
  } else {
    // on failure or if no update is available (204) there is no download url to return
    free(download_url);
    *download_url_out = NULL;
  }
  return rv;
}

int ticos_esp_port_ota_update(const sTicosOtaUpdateHandler *handler) {
  if ((handler == NULL) || (handler->handle_update_available == NULL) ||
      (handler->handle_download_complete == NULL)) {
    return TicosInternalReturnCode_InvalidInput;
  }

  char *download_url = NULL;
  int rv = prv_get_ota_update_url(&download_url);

  if ((rv != 0) || (download_url == NULL)) {
    return rv;
  }

  const bool perform_ota = handler->handle_update_available(handler->user_ctx);
  if (!perform_ota) {
    // Client decided to abort the OTA but we still set the return code to 1 to indicate a new
    // update is available.
    rv = 1;
    goto cleanup;
  }

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_https_ota_config_t config = {
    .http_config = &(esp_http_client_config_t) {
      .url = download_url,
      .timeout_ms = CONFIG_TICOS_HTTP_CLIENT_TIMEOUT_MS,
      .cert_pem = TICOS_ROOT_CERTS_PEM,
    },
    .http_client_init_cb = NULL,
    .bulk_flash_erase = false,
    .partial_http_download = false,
    .max_http_request_size = 0,
  };
#else
  esp_http_client_config_t config = {
    .url = download_url,
    .timeout_ms = CONFIG_TICOS_HTTP_CLIENT_TIMEOUT_MS,
    .cert_pem = TICOS_ROOT_CERTS_PEM,
  };
#endif

  const esp_err_t err = esp_https_ota(&config);
  if (err != ESP_OK) {
    rv = TICOS_PLATFORM_SPECIFIC_ERROR(err);
    goto cleanup;
  }

  const bool success = handler->handle_download_complete(handler->user_ctx);
  rv = success ? 1 : -1;

cleanup:
  free(download_url);
  return rv;
}

int ticos_platform_http_client_post_data(sTcsHttpClient *_client,
                                            TicosHttpClientResponseCallback callback,
                                            void *ctx) {
  if (!ticos_esp_port_data_available()) {
    return 0;  // no new chunks to send
  }

  TICOS_LOG_DEBUG("Posting Ticos Data");

  size_t buffer_size = 0;
  void *buffer = ticos_http_client_allocate_chunk_buffer(&buffer_size);
  if (buffer == NULL || buffer_size == 0) {
    TICOS_LOG_ERROR("Unable to allocate POST buffer");
    return -1;
  }

  esp_http_client_handle_t client = (esp_http_client_handle_t)_client;
  char url[TICOS_HTTP_URL_BUFFER_SIZE];
  ticos_http_build_url(url, TICOS_HTTP_CHUNKS_API_SUBPATH);
  esp_http_client_set_url(client, url);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "x-tiwater-debug","true");
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
  TICOS_LOG_DEBUG("recv: %d bytes", esp_http_client_get_content_length(client));
  if (esp_http_client_is_chunked_response(client)) {
    TICOS_LOG_DEBUG("recv: %s", _buff);
  }
  return 0;
}

int ticos_platform_http_client_wait_until_requests_completed(sTcsHttpClient *client,
                                                                uint32_t timeout_ms) {
  // No-op because ticos_platform_http_client_post_data() is synchronous
  return 0;
}

bool ticos_esp_port_wifi_connected(void) {
  wifi_ap_record_t ap_info;
  const bool connected = esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
  return connected;
}

// Similar to ticos_platform_http_client_post_data() but just posts
// whatever is pending, if anything.
int ticos_esp_port_http_client_post_data(void) {
  if (!ticos_esp_port_wifi_connected()) {
    TICOS_LOG_INFO("%s: Wifi unavailable", __func__);
    return -1;
  }

  // Check for data available first as nothing else matters if not.
  if (!ticos_esp_port_data_available()) {
    return 0;
  }

  sTcsHttpClient *http_client = ticos_http_client_create();
  if (!http_client) {
    TICOS_LOG_ERROR("Failed to create HTTP client");
    return TicosInternalReturnCode_Error;
  }
  const eTcsPostDataStatus rv = (eTcsPostDataStatus)ticos_http_client_post_data(http_client);
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

#endif /* TICOS_ESP_HTTP_CLIENT_ENABLE */
