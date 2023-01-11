#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Zephyr specific http utility for interfacing with Ticos HTTP utilities

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Installs the root CAs Ticos uses (the certs in "ticos/http/root_certs.h")
//!
//! @note: MUST be called prior to LTE network init (calling NRF's lte_lc_init_and_connect())
//!  for the settings to take effect
//!
//! @return 0 on success, else error code
int ticos_zephyr_port_install_root_certs(void);

//! Sends diagnostic data to the Ticos cloud chunks endpoint
//!
//! @note This function is blocking and will return once posting of chunk data has completed.
//!
//! For more info see https://ticos.io/data-to-cloud
int ticos_zephyr_port_post_data(void);

typedef struct TicosOtaInfo {
  // The size, in bytes, of the OTA payload.
  size_t size;
} sTicosOtaInfo;

typedef struct {
  //! Caller provided buffer to be used for the duration of the OTA lifecycle
  void *buf;
  size_t buf_len;

  //! Optional: Context for use by the caller
  void *user_ctx;

  //! Called if a new ota update is available
  //! @return true to continue, false to abort the OTA download
  bool (*handle_update_available)(const sTicosOtaInfo *info, void *user_ctx);

  //! Invoked as bytes are downloaded for the OTA update
  //!
  //! @return true to continue, false to abort the OTA download
  bool (*handle_data)(void *buf, size_t buf_len, void *user_ctx);

  //! Called once the entire ota payload has been downloaded
  bool (*handle_download_complete)(void *user_ctx);
} sTicosOtaUpdateHandler;

//! Handler which can be used to run OTA update using Ticos's Release Mgmt Infra
//! For more details see:
//!  https://ticos.io/release-mgmt
//!
//! @param handler Context with info necessary to perform an OTA. See struct
//!  definition for more details.
//!
//! @note This function is blocking. 'handler' callbacks will be invoked prior to the function
//!   returning.
//!
//! @return
//!   < 0 Error while trying to figure out if an update was available
//!     0 Check completed successfully - No new update available
//!     1 New update is available and handlers were invoked
int ticos_zephyr_port_ota_update(const sTicosOtaUpdateHandler *handler);

//! Query Ticos's Release Mgmt Infra for an OTA update
//!
//! @param download_url populated with a string containing the download URL to use
//! if an OTA update is available.
//!
//! @note After use, ticos_zephyr_port_release_download_url() must be called
//!  to free the memory where the download URL is stored.
//!
//! @return
//!   < 0 Error while trying to figure out if an update was available
//!     0 Check completed successfully - No new update available
//!     1 New update is available and download_url has been populated with
//!       the url to use for download
int ticos_zephyr_port_get_download_url(char **download_url);

//! Releases the memory returned from ticos_zephyr_port_get_download_url()
int ticos_zephyr_port_release_download_url(char **download_url);

//!
//! Utility functions for manually posting ticos data.

//! Context structure used to carry state information about the HTTP connection
typedef struct {
  int sock_fd;
  struct zsock_addrinfo *res;
} sTicosHttpContext;

//! Open a socket to the Ticos chunks upload server
//!
//! @param ctx If the socket is opened successfully, this will be populated with
//! the connection state for the other HTTP functions below
//!
//! @note After use, ticos_zephyr_port_http_close_socket() must be called
//!  to close the socket and free any associated memory
//!
//! @return
//!   0 : Success
//! < 0 : Error
int ticos_zephyr_port_http_open_socket(sTicosHttpContext *ctx);

//! Close a socket previously opened with
//! ticos_zephyr_port_http_open_socket()
void ticos_zephyr_port_http_close_socket(sTicosHttpContext *ctx);

//! Test if the socket is open
bool ticos_zephyr_port_http_is_connected(sTicosHttpContext *ctx);

//! Identical to ticos_zephyr_port_post_data() but uses the already-opened
//! socket to send data
//!
//! @param ctx Connection context previously opened with
//! ticos_zephyr_port_http_open_socket()
void ticos_zephyr_port_http_upload_sdk_data(sTicosHttpContext *ctx);

//! Similar to ticos_zephyr_port_http_upload_sdk_data(), but instead of using
//! the SDK packetizer functions under the hood, send the data passed into this
//! function.
//!
//! Typically this function is used to send data from pre-packetized data; for
//! example, data that may have been stored outside of the Ticos SDK
//! internally-managed buffers, or data coming from an external source (another
//! chip running the Ticos SDK).
//!
//! @param ctx Connection context previously opened with
//! ticos_zephyr_port_http_open_socket()
//!
//! @return
//!   0 : Success
//! < 0 : Error
int ticos_zephyr_port_http_post_chunk(sTicosHttpContext *ctx, void *p_data, size_t data_len);

#ifdef __cplusplus
}
#endif
