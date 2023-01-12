//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "ticos/http/utils.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/core/platform/device_info.h"
#include "ticos/http/http_client.h"

//! Default buffer size for the URL-encoded device info parameters. This may
//! need to be set higher by the user if there are particularly long device info
//! strings
#ifndef TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN
#define TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN (48)
#endif

static bool prv_write_msg(TcsHttpClientSendCb write_callback, void *ctx,
                          const char *msg, size_t msg_len, size_t max_len) {
  if (msg_len >= max_len) {
    return false;
  }

  return write_callback(msg, msg_len, ctx);
}

static bool prv_write_crlf(TcsHttpClientSendCb write_callback, void *ctx) {
  #define END_HEADER_SECTION "\r\n"
  const size_t end_hdr_section_len = TICOS_STATIC_STRLEN(END_HEADER_SECTION);
  return write_callback(END_HEADER_SECTION, end_hdr_section_len, ctx);
}

// NB: All HTTP/1.1 requests must provide a Host Header
//    https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Host
static bool prv_write_host_hdr(TcsHttpClientSendCb write_callback, void *ctx,
                               const void *host, size_t host_len) {
  #define HOST_HDR_BEGIN "Host:"
  const size_t host_hdr_begin_len = TICOS_STATIC_STRLEN(HOST_HDR_BEGIN);
  if (!write_callback(HOST_HDR_BEGIN, host_hdr_begin_len, ctx)) {
    return false;
  }

  if (!write_callback(host, host_len, ctx)) {
    return false;
  }

  return prv_write_crlf(write_callback, ctx);
}

static bool prv_write_user_agent_hdr(TcsHttpClientSendCb write_callback, void *ctx) {
  #define USER_AGENT_HDR "User-Agent:TicosSDK/0.4.2\r\n"
  const size_t user_agent_hdr_len = TICOS_STATIC_STRLEN(USER_AGENT_HDR);
  return write_callback(USER_AGENT_HDR, user_agent_hdr_len, ctx);
}

static bool prv_write_project_key_hdr(TcsHttpClientSendCb write_callback, void *ctx) {
  #define PROJECT_KEY_HDR_BEGIN "Ticos-Project-Key:"
  const size_t project_key_begin_len = TICOS_STATIC_STRLEN(PROJECT_KEY_HDR_BEGIN);
  if (!write_callback(PROJECT_KEY_HDR_BEGIN, project_key_begin_len, ctx)) {
    return false;
  }

  const char *project_key = g_tcs_http_client_config.api_key;
  size_t project_key_len = strlen(project_key);

  if (!write_callback(project_key, project_key_len, ctx)) {
    return false;
  }

  return prv_write_crlf(write_callback, ctx);
}

bool ticos_http_start_chunk_post(
    TcsHttpClientSendCb write_callback, void *ctx, size_t content_body_length) {
  // Request built will look like this:
  //  POST /api/v0/chunks/<device_serial> HTTP/1.1\r\n
  //  Host:chunks.ticos.com\r\n
  //  User-Agent: TicosSDK/0.4.2\r\n
  //  Ticos-Project-Key:<PROJECT_KEY>\r\n
  //  Content-Type:application/octet-stream\r\n
  //  Content-Length:<content_body_length>\r\n
  //  \r\n

  sTicosDeviceInfo device_info;
  ticos_platform_get_device_info(&device_info);

  char buffer[100];
  const size_t max_msg_len = sizeof(buffer);
  size_t msg_len = (size_t)snprintf(buffer, sizeof(buffer),
                                    "POST /chunks/%s HTTP/1.1\r\n",
                                    device_info.device_serial);
  if (!prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len)) {
    return false;
  }

  const char *host = TICOS_HTTP_GET_CHUNKS_API_HOST();
  const size_t host_len = strlen(host);
  if (!prv_write_host_hdr(write_callback, ctx, host, host_len)) {
    return false;
  }

  if (!prv_write_user_agent_hdr(write_callback, ctx)) {
    return false;
  }

  if (!prv_write_project_key_hdr(write_callback, ctx)) {
    return false;
  }

  #define CONTENT_TYPE "Content-Type:application/octet-stream\r\n"
  const size_t content_type_len = TICOS_STATIC_STRLEN(CONTENT_TYPE);
  if (!write_callback(CONTENT_TYPE, content_type_len, ctx)) {
    return false;
  }

  msg_len = (size_t)snprintf(buffer, sizeof(buffer), "Content-Length:%d\r\n",
                             (int)content_body_length);

  return prv_write_msg(write_callback, ctx, buffer, msg_len, max_msg_len) &&
      prv_write_crlf(write_callback, ctx);
}

static bool prv_write_qparam(TcsHttpClientSendCb write_callback, void *ctx,
                             const void *name, size_t name_strlen, const char *value) {
  return write_callback("&", 1, ctx) &&
      write_callback(name, name_strlen, ctx) &&
      write_callback("=", 1, ctx) &&
      write_callback(value, strlen(value), ctx);
}

bool ticos_http_get_latest_ota_payload_url(TcsHttpClientSendCb write_callback, void *ctx) {
  // Request built will look like this:
  //  GET /api/v0/releases/latest/url&device_serial=<>&hardware_version=<>&software_type=<>&current_version=<> HTTP/1.1\r\n
  //  Host:<ota download url host>\r\n
  //  User-Agent: TicosSDK/0.4.2\r\n
  //  Ticos-Project-Key:<PROJECT_KEY>\r\n
  //  \r\n

  #define LATEST_REQUEST_LINE_BEGIN "GET /api/v0/releases/latest/url?"
  const size_t request_line_begin_len = TICOS_STATIC_STRLEN(LATEST_REQUEST_LINE_BEGIN);
  if (!write_callback(LATEST_REQUEST_LINE_BEGIN, request_line_begin_len, ctx)) {
    return false;
  }

  sTicosDeviceInfo device_info = { 0 };
  ticos_platform_get_device_info(&device_info);

  #define DEVICE_SERIAL_QPARAM "device_serial"
  #define HARDWARE_VERSION_QPARAM "hardware_version"
  #define SOFTWARE_TYPE_QPARAM "software_type"
  #define CURRENT_VERSION_QPARAM "current_version"

  #define TICOS_STATIC_STRLEN(s) (sizeof(s) - 1)

  const struct qparam_values_s {
    const char *name;
    size_t name_strlen;
    const char *value;
  } qparam_values[] = {
    {
      DEVICE_SERIAL_QPARAM,
      TICOS_STATIC_STRLEN(DEVICE_SERIAL_QPARAM),
      device_info.device_serial,
    },
    {
      HARDWARE_VERSION_QPARAM,
      TICOS_STATIC_STRLEN(HARDWARE_VERSION_QPARAM),
      device_info.hardware_version,
    },
    {
      SOFTWARE_TYPE_QPARAM,
      TICOS_STATIC_STRLEN(SOFTWARE_TYPE_QPARAM),
      device_info.software_type,
    },
    {
      CURRENT_VERSION_QPARAM,
      TICOS_STATIC_STRLEN(CURRENT_VERSION_QPARAM),
      device_info.software_version,
    },
  };

  // URL encode the qparam values before writing them
  for (size_t i = 0; i < TICOS_ARRAY_SIZE(qparam_values); i++) {
    const struct qparam_values_s *qparam_value = &qparam_values[i];

    char qparam_encoded_buffer[TICOS_DEVICE_INFO_URL_ENCODED_MAX_LEN];
    int rv = ticos_http_urlencode(qparam_value->value, strlen(qparam_value->value),
                                     qparam_encoded_buffer, sizeof(qparam_encoded_buffer));
    if (rv != 0) {
      TICOS_LOG_ERROR("Failed to URL encode qparam value: %s", qparam_value->value);
      return false;
    }

    if (!prv_write_qparam(write_callback, ctx, qparam_value->name, qparam_value->name_strlen,
                          qparam_encoded_buffer)) {
      return false;
    }
  }

  #define LATEST_REQUEST_LINE_END " HTTP/1.1\r\n"
  const size_t request_line_end_len = TICOS_STATIC_STRLEN(LATEST_REQUEST_LINE_END);
  if (!write_callback(LATEST_REQUEST_LINE_END, request_line_end_len, ctx)) {
    return false;
  }

  const char *host = TICOS_HTTP_GET_DEVICE_API_HOST();
  const size_t host_len = strlen(host);
  if (!prv_write_host_hdr(write_callback, ctx, host, host_len)) {
    return false;
  }

  if (!prv_write_user_agent_hdr(write_callback, ctx)) {
    return false;
  }

  if (!prv_write_project_key_hdr(write_callback, ctx)) {
    return false;
  }

  return prv_write_crlf(write_callback, ctx);
}

static bool prv_is_number(char c) {
  return ((c) >= '0' && (c) <= '9');
}

static size_t prv_count_spaces(const char *line, size_t start_offset, size_t total_len) {
  size_t spaces_found = 0;
  for (; start_offset < total_len; start_offset++) {
    if (line[start_offset] != ' ') {
      break;
    }
    spaces_found++;
  }

  return spaces_found;
}

static char prv_lower(char in) {
  char maybe_lower_c = in | 0x20;
  if (maybe_lower_c >= 'a' && maybe_lower_c <= 'z') {
    // return lower case if value is actually in A-Z, a-z range
    return maybe_lower_c;
  }

  return in;
}

// depending on the libc used, strcasecmp isn't always available so let's
// use a simple variant here
static bool prv_strcasecmp(const char *line, const char *search_str, size_t str_len) {
  for (size_t idx = 0; idx < str_len; idx++) {
    char lower_c = prv_lower(line[idx]);

    if (lower_c != search_str[idx]) {
      return false;
    }
  }

  return true;
}

static int prv_str_to_dec(const char *buf, size_t buf_len, int *value_out) {
  int result = 0;
  size_t idx = 0;
  for (; idx < buf_len; idx++) {
    char c = buf[idx];
    if (c == ' ') {
      break;
    }

    if (!prv_is_number(c)) {
      // unexpected character encountered
      return -1;
    }

    int digit = c - '0';

    // there's no limit to the size of a Content-Length value per specification:
    // https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.2
    //
    // status code is required to be 3 digits per:
    // https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.2
    //
    // any value that we can't fit in our variable is an error
    if ((INT_MAX / 10) < (result + digit)) {
      return -1; // result will overflow
    }

    result = (result * 10) + digit;
  }

  *value_out = result;
  return (int)idx;
}

//! @return true if parsing was successful, false if a parse error occurred
//! @note The only header we are interested in for the simple ticos response
//! parser is "Content-Length" to figure out how long the body is so that's all
//! this parser looks for
static bool prv_parse_header(
    char *line, size_t len, int *content_length_out) {
  #define CONTENT_LENGTH "content-length"
  const size_t content_hdr_len = TICOS_STATIC_STRLEN(CONTENT_LENGTH);
  if (len < content_hdr_len) {
    return true;
  }

  size_t idx = 0;
  if (!prv_strcasecmp(line, CONTENT_LENGTH, content_hdr_len)) {
    return true;
  }

  idx += content_hdr_len;

  idx += prv_count_spaces(line, idx, len);
  if (line[idx] != ':') {
    return false;
  }
  idx++;

  idx += prv_count_spaces(line, idx, len);

  const int bytes_processed = prv_str_to_dec(&line[idx], len - idx, content_length_out);
  // should find at least one digit
  return (bytes_processed  > 0);
}

static bool prv_parse_status_line(char *line, size_t len, int *http_status) {
  #define HTTP_VERSION "HTTP/1."
  const size_t http_ver_len = TICOS_STATIC_STRLEN(HTTP_VERSION);
  if (len < http_ver_len) {
    return false;
  }
  if (memcmp(line, HTTP_VERSION, http_ver_len) != 0) {
    return false;
  }
  size_t idx = http_ver_len;
  if (len < idx + 1) {
    return false;
  }

  // now we should have single byte minor version
  if (!prv_is_number(line[idx])) {
    return false;
  }
  idx++;

  // now we should have at least one space
  const size_t num_spaces = prv_count_spaces(line, idx, len);
  if (num_spaces == 0) {
    return false;
  }
  idx += num_spaces;

  const size_t status_code_num_digits = 3;
  size_t status_code_end = idx + status_code_num_digits;
  if (len < status_code_end ) {
    return false;
  }

  const int bytes_processed = prv_str_to_dec(&line[idx], status_code_end, http_status);

  // NB: the remainder line is the "Reason-Phrase" which we don't care about
  return (bytes_processed == (int)status_code_num_digits);
}

static bool prv_is_cr_lf(char *buf) {
  return buf[0] == '\r' && buf[1] == '\n';
}

static bool prv_parse_http_response(sTicosHttpResponseContext *ctx, const void *data,
                                    size_t data_len, bool parse_header_only) {
  ctx->data_bytes_processed = 0;

  const char *chars = (const char *)data;
  char *line_buf = &ctx->line_buf[0];
  for (size_t i = 0; i < data_len; i++, ctx->data_bytes_processed++) {
    const char c = chars[i];
    if (ctx->phase == kTcsHttpParsePhase_ExpectingBody) {
      if (parse_header_only) {
        return true;
      }

      // Just eat the message body so we can handle response lengths of arbitrary size
      ctx->content_received++;

      if (ctx->line_len < (sizeof(ctx->line_buf) - 1)) {
        line_buf[ctx->line_len] = c;
        ctx->line_len++;
      }

      bool done = (ctx->content_received == ctx->content_length);
      if (!done) {
        continue;
      }
      ctx->line_buf[ctx->line_len] = '\0';
      ctx->http_body = ctx->line_buf;
      ctx->data_bytes_processed++;
      return done;
    }

    if (ctx->line_len >= sizeof(ctx->line_buf)) {
      if (ctx->phase == kTcsHttpParsePhase_ExpectingHeader) {
        // We want to truncate headers at index sizeof(line_buf)-2
        // so we can place the source's CR/LF sequence at the end
        // when the source is finally exhausted.
        static const size_t lf_idx = sizeof(ctx->line_buf) - 2;
        static const size_t cr_idx = sizeof(ctx->line_buf) - 1;
        if (c == '\r') {
          ctx->line_buf[lf_idx] = c;
        } else if (c == '\n') {
          ctx->line_buf[cr_idx] = c;
        }
      } else {
        // It's too long so set a parse error and return done.
        ctx->parse_error = TcsHttpParseStatus_HeaderTooLongError;
        return true;
      }
    } else {
      line_buf[ctx->line_len] = c;
      ctx->line_len++;
    }

    if (ctx->line_len < 2) {
      continue;
    }

    const size_t len = ctx->line_len - 2;
    if (prv_is_cr_lf(&line_buf[len])) {
      ctx->line_len  = 0;
      line_buf[len] = '\0';

      // The first line in a http response is the HTTP "Status-Line"
      if (ctx->phase == kTcsHttpParsePhase_ExpectingStatusLine) {
        if (!prv_parse_status_line(line_buf, len, &ctx->http_status_code)) {
          ctx->parse_error = TcsHttpParseStatus_ParseStatusLineError;
          return true;
        }
        ctx->phase = kTcsHttpParsePhase_ExpectingHeader;
      } else if (ctx->phase == kTcsHttpParsePhase_ExpectingHeader) {
        if (!prv_parse_header(line_buf, len, &ctx->content_length)) {
          ctx->parse_error = TcsHttpParseStatus_ParseHeaderError;
          return true;
        }

        if (len != 0) {
          continue;
        }
        // We've reached the end of headers marker
        if (ctx->content_length == 0) {
          // no body to read
          return true;
        }
        ctx->phase = kTcsHttpParsePhase_ExpectingBody;
      }
    }
  }
  return false;
}

bool ticos_http_parse_response(
    sTicosHttpResponseContext *ctx, const void *data, size_t data_len) {
  const bool parse_header_only = false;
  return prv_parse_http_response(ctx, data, data_len, parse_header_only);
}

bool ticos_http_parse_response_header(
    sTicosHttpResponseContext *ctx, const void *data, size_t data_len) {
  const bool parse_header_only = true;
  return prv_parse_http_response(ctx, data, data_len, parse_header_only);
}

static bool prv_find_first_occurrence(const char *line, size_t total_len,
                                     char c, size_t *offset_out) {
  for (size_t offset = 0; offset < total_len; offset++) {
    if (line[offset] == c) {
      *offset_out = offset;
      return true;
    }
  }

  return false;
}

static bool prv_find_last_occurrence(const char *line, size_t total_len,
                                    char c, size_t *offset_out) {
  for (int offset = (int)total_len - 1; offset >= 0; offset--) {
    if (line[offset] == c) {
      *offset_out = (size_t)offset;
      return true;
    }
  }

  return false;
}

static bool prv_startswith_str(const char *match, size_t match_len,
                               const void *uri, size_t total_len) {
  if (total_len < match_len) {
    return false;
  }


  return prv_strcasecmp(uri, match, match_len);
}

static size_t prv_is_http_or_https_scheme(const void *uri, size_t total_len,
                                          eTicosUriScheme *scheme,
                                          uint32_t *default_port) {
  #define HTTPS_SCHEME_WITH_AUTHORITY "https://"
  const size_t https_scheme_with_authority_len =
      TICOS_STATIC_STRLEN(HTTPS_SCHEME_WITH_AUTHORITY);
  if (prv_startswith_str(HTTPS_SCHEME_WITH_AUTHORITY, https_scheme_with_authority_len,
                         uri, total_len)) {
    *scheme = kTicosUriScheme_Https;
    *default_port = 443;
    return https_scheme_with_authority_len;
  }

  #define HTTP_SCHEME_WITH_AUTHORITY "http://"
  const size_t http_scheme_with_authority_len =
      TICOS_STATIC_STRLEN(HTTP_SCHEME_WITH_AUTHORITY);
  if (prv_startswith_str(HTTP_SCHEME_WITH_AUTHORITY, http_scheme_with_authority_len,
                         uri, total_len)) {
    *scheme = kTicosUriScheme_Http;
    *default_port = 80;
    return http_scheme_with_authority_len;
  }

  *scheme = kTicosUriScheme_Unrecognized;
  return 0;
}

bool ticos_http_parse_uri(
    const char *uri, size_t uri_len, sTicosUriInfo *uri_info_out) {
  size_t host_len = uri_len; // we will adjust as we parse the uri

  eTicosUriScheme scheme;
  uint32_t port = 0;
  const size_t bytes_consumed = prv_is_http_or_https_scheme(
      uri, host_len, &scheme, &port);
  if (bytes_consumed == 0) {
    return false;
  }
  host_len -= bytes_consumed;

  const char *authority = uri + bytes_consumed;
  // Authority ends with a "/" when followed by a "path" or is the length of the string
  size_t offset;
  const char *path = NULL;
  size_t path_len = 0;
  if (prv_find_first_occurrence(authority, host_len, '/', &offset)) {
    path = &authority[offset];
    path_len = host_len - offset;
    host_len = offset;
  }

  if (prv_find_first_occurrence(authority, host_len, '@', &offset)) {
    offset++; // skip past the '@' - no use for username/password today
    if (host_len == offset) {
      return false;
    }

    authority += offset;
    host_len -= offset;
  }

  // are we dealing with an IP-Literal?
  size_t port_begin_search_offset = 0;
  if (authority[0] == '[') {
    if (!prv_find_last_occurrence(authority, host_len, ']', &port_begin_search_offset)) {
      return false;
    }
  }

  // was a port number included?
  if (prv_find_last_occurrence(authority, host_len, ':', &offset)) {
    if (offset >= port_begin_search_offset) {

      const size_t port_begin_offset = offset + 1 /* skip ':' */;
      // recover the port
      int dec_num;
      const int bytes_processed =
          prv_str_to_dec(&authority[port_begin_offset], host_len - port_begin_offset, &dec_num);
      if (bytes_processed <= 0) {
        return false;
      }
      port = (uint32_t)dec_num;
      host_len = offset;
    }
  }

  if (host_len == 0) {
    return false; // no host name located!
  }

  *uri_info_out = (sTicosUriInfo) {
    .scheme = scheme,
    .host = authority,
    .host_len = host_len,
    .path = path,
    .path_len = path_len,
    .port = port,
  };
  return true;
}

bool ticos_http_get_ota_payload(TcsHttpClientSendCb write_callback, void *ctx,
                                   const char *url, size_t url_len) {
  // Request built will look like this:
  //  GET <Request-URI from url> HTTP/1.1\r\n
  //  Host:<Host from url>\r\n
  //  User-Agent: TicosSDK/0.4.2\r\n
  //  \r\n

  sTicosUriInfo info;
  if (!ticos_http_parse_uri(url, url_len, &info)) {
    return false;
  }

  #define DOWNLOAD_REQUEST_LINE_BEGIN "GET "
  const size_t download_line_begin_len = TICOS_STATIC_STRLEN(DOWNLOAD_REQUEST_LINE_BEGIN);
  if (!write_callback(DOWNLOAD_REQUEST_LINE_BEGIN, download_line_begin_len, ctx)) {
    return false;
  }

  bool success = (info.path != NULL) ? write_callback(info.path, info.path_len, ctx) :
                                       write_callback("/", 1, ctx);
  if (!success) {
    return false;
  }

  #define DOWNLOAD_REQUEST_LINE_END " HTTP/1.1\r\n"
  const size_t download_request_line_end_len = TICOS_STATIC_STRLEN(DOWNLOAD_REQUEST_LINE_END);
  if (!write_callback(DOWNLOAD_REQUEST_LINE_END, download_request_line_end_len, ctx)) {
    return false;
  }

  if (!prv_write_host_hdr(write_callback, ctx, info.host, info.host_len)) {
    return false;
  }


  if (!prv_write_user_agent_hdr(write_callback, ctx)) {
    return false;
  }

  return prv_write_crlf(write_callback, ctx);
}

static bool prv_is_unreserved(char c) {
  return isalnum((uint8_t)c) || c == '-' || c == '_' || c == '.' || c == '~';
}

bool ticos_http_needs_escape(const char *str, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!prv_is_unreserved(str[i])) {
      return true;
    }
  }
  return false;
}

int ticos_http_urlencode(const char *inbuf, size_t inbuf_len, char *outbuf, size_t outbuf_len) {
  // null check
  if (!inbuf || !outbuf || !inbuf_len || !outbuf_len) {
    return -1;
  }

  while (inbuf_len--) {
    if (outbuf_len < 2) {
      // ran out of room before encoding the full input, error. there needs to
      // be 1 spare character for null term
      return -1;
    }
    char c = *inbuf++;
    if (prv_is_unreserved(c)) {
      *outbuf++ = c;
      outbuf_len--;
    } else {
      if (outbuf_len < 4) {
        // not enough room for encoded character and null term
        return -1;
      }
      // paste encoding (+ null term)
      snprintf(outbuf, 4, "%%%02X", (uint8_t)c);
      outbuf += 3;
      outbuf_len -= 3;
    }
  }

  // null terminate
  *outbuf = '\0';

  return 0;
}
