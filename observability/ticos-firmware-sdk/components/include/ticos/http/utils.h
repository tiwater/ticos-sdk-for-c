#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A collection of HTTP utilities *solely* for interacting with the Ticos REST API including
//!  - An API for generating HTTP Requests to Ticos REST API
//!  - An API for incrementally parsing HTTP Response Data [1]
//!
//! @note The expectation is that a typical application making use of HTTP will have a HTTP client
//! and parsing implementation already available to leverage.  This module is provided as a
//! reference and convenience implementation for very minimal environments.
//!
//! [1]: For more info on embedded-C HTTP parsing options, the following commit
//! in the Zephyr RTOS is a good starting point: https://ticos.io/35RWWwp

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Writer invoked by calls to "ticos_http_start_chunk_post"
//!
//! For example, this would be where a user of the API would make a call to send() to push data
//! over a socket
typedef bool(*TcsHttpClientSendCb)(const void *data, size_t data_len, void *ctx);

//! Builds the HTTP 'Request-Line' and Headers for a POST to the Ticos Chunk Endpoint
//!
//! @note Upon completion of this call, a caller then needs to send the raw data received from
//! ticos_packetizer_get_next() out over the active connection.
//!
//! @param callback The callback invoked to send post request data.
//! @param ctx A user specific context that gets passed to 'callback' invocations.
//! @param content_body_length The length of the chunk payload to be sent. This value
//!  will be populated in the HTTP "Content-Length" header.
//!
//! @return true if the post was successful, false otherwise
bool ticos_http_start_chunk_post(
    TcsHttpClientSendCb callback, void *ctx, size_t content_body_length);

//! Builds the HTTP GET request to query the Ticos cloud to see if a new OTA Payload is available
//!
//! For more details about release management and OTA payloads in general, check out:
//!  https://ticos.io/release-mgmt
//!
//! @param callback The callback invoked to send post request data.
//! @param ctx A user specific context that gets passed to 'callback' invocations.
//!
//! @return true if sending the request was successful, false otherwise. On success,
//!  the request response body can be read where the following HTTP Status Codes
//!  are expected:
//!    200: A new firmware (OTA Payload) is available and the response contains a url for it
//!    204: No new firmware is available
//!    4xx, 5xx: Error
bool ticos_http_get_latest_ota_payload_url(TcsHttpClientSendCb write_callback, void *ctx);

//! Builds the HTTP GET request to download a Firmware OTA Payload
//!
//! @param callback The callback invoked to send post request data.
//! @param ctx A user specific context that gets passed to 'callback' invocations.
//! @param url The ota payload url returned in the body of the HTTP request constructed with
//!   ticos_http_get_latest_ota_payload_url().
//! @param url_len The string length of the url param
//!
//! @return true if sending the request was successful, false otherwise. On success,
//!   the request response body can be read where the Content-Length will contain the size
//!   of the OTA payload and the message-body will be the OTA Payload that was uploaded via
//!   the Ticos UI
bool ticos_http_get_ota_payload(TcsHttpClientSendCb write_callback, void *ctx,
                                   const char *url, size_t url_len);

typedef enum TcsHttpParseStatus {
  kTcsHttpParseStatus_Ok = 0,
  TcsHttpParseStatus_ParseStatusLineError,
  TcsHttpParseStatus_ParseHeaderError,
  TcsHttpParseStatus_HeaderTooLongError,
} eTcsHttpParseStatus;

typedef enum TcsHttpParsePhase {
  kTcsHttpParsePhase_ExpectingStatusLine = 0,
  kTcsHttpParsePhase_ExpectingHeader,
  kTcsHttpParsePhase_ExpectingBody,
} eTcsHttpParsePhase;

typedef struct {
  //! true if a error occurred trying to parse the response
  eTcsHttpParseStatus parse_error;
  //! populated with the status code returned as part of the response
  int http_status_code;
  //! Pointer to http_body, may be truncated but always NULL terminated.
  //! This should only be used for debug purposes
  const char *http_body;
  //! The number of bytes processed by the last invocation of
  //! "ticos_http_parse_response" or "ticos_http_parse_response_header"
  int data_bytes_processed;
  //! Populated with the Content-Length found in the HTTP Response Header
  //! Valid upon parsing completion if no parse_error was returned
  int content_length;

  // For internal use only
  eTcsHttpParsePhase phase;
  int content_received;
  size_t line_len;
  char line_buf[128];
} sTicosHttpResponseContext;


//! A *minimal* HTTP response parser for Ticos API calls
//!
//! @param ctx The context to be used while a parsing is in progress. It's
//!  expected that when a user first calls this function the context will be
//!  zero initialized.
//! @param data The data to parse
//! @param data_len The length of the data to parse
//! @return True if parsing completed or false if more data is needed for the response
//!   Upon completion the 'parse_error' & 'http_status_code' fields can be checked
//!   within the 'ctx' for the results
bool ticos_http_parse_response(
    sTicosHttpResponseContext *ctx, const void *data, size_t data_len);

//! Same as ticos_http_parse_response but only parses the response header
//!
//! This API can be useful when the message body contains information further action
//! is taken on (i.e an OTA Payload)
//!
//! @note Specifically, all data up to the "message-body" is consumed by the parser
//!    Response      = Status-Line
//!                    *(( general-header
//!                     | response-header
//!                     | entity-header ) CRLF)
//!                    CRLF
//!                    [ message-body ]          ; **NOT Consumed**
bool ticos_http_parse_response_header(
    sTicosHttpResponseContext *ctx, const void *data, size_t data_len);

typedef enum {
  kTicosUriScheme_Unrecognized = 0,

  kTicosUriScheme_Http,
  kTicosUriScheme_Https,
} eTicosUriScheme;

typedef struct {
  //! Protocol detected (Either HTTPS or HTTP)
  eTicosUriScheme scheme;

  //! The 'host' component of the uri:
  //!   https://tools.ietf.org/html/rfc3986#section-3.2.2
  //! @note: NOT nul-terminated
  const void *host;
  size_t host_len;

  //! Port to use for connection
  //! @note if no port is specified in URI, default for scheme will be
  //! populated (i.e 80 for http & 443 for https)
  uint32_t port;

  //! The 'path' component of the uri:
  //!   https://tools.ietf.org/html/rfc3986#section-3.3
  //! @note: Path will be NULL when empty in URI. Like host, when
  //!  path is populated, it is not terminated with a nul character.
  const void *path;
  size_t path_len;
} sTicosUriInfo;

//! A *minimal* parser for HTTP/HTTPS URIs
//!
//! @note API performs no copies or mallocs. Instead uri_info_out contains
//!  pointers back to the original uri provided and lengths of fields parsed.
//!
//! @note For more details about URIs in general, check out the RFC
//!   https://tools.ietf.org/html/rfc3986#section-1.1.3
//! @note this function parses URIs with 'scheme's of "http" or "https"
//!   URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
//!
//! @param uri The URI to try and parse.
//! @param uri_len The string length of the uri
//! @param[out] uri_info_out On successful parse, populated with various pieces of information
//!   See 'sTicosUriInfo' for more details.
//!
//! @return true if parse was successful and uri_info_out is populated, false  otherwise
bool ticos_http_parse_uri(const char *uri, size_t uri_len, sTicosUriInfo *uri_info_out);

//! Check if a string contains any characters that require URL escaping
//!
//! @param str the string to check
//! @param len length of str
//!
//! @return true if any characters in str require URL escaping, false otherwise
bool ticos_http_needs_escape(const char *str, size_t len);

//! URL encode a string. See https://www.ietf.org/rfc/rfc3986.html#section-2.1
//!
//! @param inbuf String to encode; must be null-terminated
//! @param[out] outbuf Endcoded string. Should be sized to fit possible encoding
//! overhead, eg 3 * strlen(inbuf)
//! @param outbuf_len Size of outbuf
//!
//! @return 0 if encoding was successful, non zero otherwise
int ticos_http_urlencode(const char *inbuf, size_t inbuf_len, char *outbuf, size_t outbuf_len);

#ifdef __cplusplus
}
#endif
