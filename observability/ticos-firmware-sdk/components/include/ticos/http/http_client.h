#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! API when using the Ticos HTTP Client

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"

#define TICOS_HTTP_URL_BUFFER_SIZE (128)

#define TICOS_HTTP_CHUNKS_API_PREFIX "/"
#define TICOS_HTTP_CHUNKS_API_SUBPATH "chunks"
#define TICOS_HTTP_PROJECT_KEY_HEADER "Ticos-Project-Key"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  //! The API host to use, NULL to use the default host.
  const char *host;
  //! The TCP port to use or 0 to use the default port as defined by TICOS_HTTP_CHUNKS_API_PORT.
  uint16_t port;
} sTicosHttpApi;

//! Configuration of the Ticos HTTP client.
typedef struct TcsHttpClientConfig {
  //! The project key. This is a mandatory field.
  //! Go to app.ticos.com, then navigate to "Settings" to find your key.
  const char *api_key;
  //! When false, TLS/https will be used, otherwise plain text http will be used.
  bool disable_tls;
  //! Route used to send packetized data ("chunks") to the Ticos cloud for reassembly and
  //! processing. See https://ticos.io/data-to-cloud for more details.
  sTicosHttpApi chunks_api;
  //! Route used to get information from the Ticos cloud pertaining to a device in your fleet.
  //! For example, the latest firmware release available.
  sTicosHttpApi device_api;
} sTcsHttpClientConfig;

//! Global configuration of the Ticos HTTP client.
//! See @ref sTcsHttpClientConfig for information about each of the fields.
extern sTcsHttpClientConfig g_tcs_http_client_config;

//! Convenience macros to get the currently configured Chunks API hostname & Port
#define TICOS_HTTP_GET_CHUNKS_API_HOST() \
  (g_tcs_http_client_config.chunks_api.host ? g_tcs_http_client_config.chunks_api.host : \
                                               TICOS_HTTP_CHUNKS_API_HOST)
#define TICOS_HTTP_GET_CHUNKS_API_PORT() \
    (g_tcs_http_client_config.chunks_api.port ? g_tcs_http_client_config.chunks_api.port : \
                                                 TICOS_HTTP_APIS_DEFAULT_PORT)


//! Convenience macros to get the currently configured Device API hostname & Port
#define TICOS_HTTP_GET_DEVICE_API_HOST() \
  (g_tcs_http_client_config.device_api.host ? g_tcs_http_client_config.device_api.host : \
                                               TICOS_HTTP_DEVICE_API_HOST)
#define TICOS_HTTP_GET_DEVICE_API_PORT() \
    (g_tcs_http_client_config.device_api.port ? g_tcs_http_client_config.device_api.port : \
                                                 TICOS_HTTP_APIS_DEFAULT_PORT)

//! Returns the "scheme" part of the URI based on client configuration
#define TICOS_HTTP_GET_SCHEME() \
  (g_tcs_http_client_config.disable_tls ? "http" : "https")

//! Forward declaration of a HTTP client.
typedef struct TcsHttpClient sTcsHttpClient;

//! Writes a Ticos API URL for the specified subpath.
//! @param url_buffer Buffer where the URL should be written.
//! @param subpath Subpath to use.
//! @return true if the buffer was filled, false otherwise
bool ticos_http_build_url(char url_buffer[TICOS_HTTP_URL_BUFFER_SIZE], const char *subpath);

//! Creates a new HTTP client. A client can be reused across requests.
//! This way, the cost of connection set up to the server will be shared with multiple requests.
//! @return The newly created client or NULL in case of an error.
//! @note Make sure to call ticos_platform_http_client_destroy() to close and clean up resources.
sTcsHttpClient *ticos_http_client_create(void);

typedef enum {
  kTcsPostDataStatus_Success = 0,
  kTcsPostDataStatus_NoDataFound = 1,
} eTcsPostDataStatus;

//! Posts Ticos data that is pending transmission to Ticos's services over HTTP.
//!
//! @return kTcsPostDataStatus_Success on success, kTcsPostDataStatus_NoDataFound
//! if no data was found or else an error code.
int ticos_http_client_post_data(sTcsHttpClient *client);

//! Create a http client, post a chunk of data and then teardown the connection
//!
//! @return kTcsPostDataStatus_Success on success, kTcsPostDataStatus_NoDataFound
//! if no data was found or else an error code.
int ticos_http_client_post_chunk(void);

//! Waits until pending requests have been completed.
//! @param client The http client.
//! @return 0 on success, else error code
int ticos_http_client_wait_until_requests_completed(sTcsHttpClient *client,
                                                       uint32_t timeout_ms);

//! Destroys a HTTP client that was previously created using ticos_platform_http_client_create().
int ticos_http_client_destroy(sTcsHttpClient *client);

#ifdef __cplusplus
}
#endif
