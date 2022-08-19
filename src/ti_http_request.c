// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_http_header_validation_private.h"
#include "ti_http_private.h"
#include "ti_span_private.h"

#include <ti_http.h>
#include <ti_http_transport.h>
#include <ti_precondition.h>
#include <ti_http_internal.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include <assert.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_http_request_init(
    ti_http_request* out_request,
    ti_context* context,
    ti_span method,
    ti_span url,
    int32_t url_length,
    ti_span headers_buffer,
    ti_span body)
{
  _ti_PRECONDITION_NOT_NULL(out_request);
  _ti_PRECONDITION_VALID_SPAN(method, 1, false);
  _ti_PRECONDITION_VALID_SPAN(url, 1, false);
  _ti_PRECONDITION_VALID_SPAN(headers_buffer, 0, false);

  int32_t query_start = 0;
  uint8_t const* const ptr = ti_span_ptr(url);
  for (; query_start < url_length; ++query_start)
  {
    uint8_t next_byte = ptr[query_start];
    if (next_byte == '?')
    {
      break;
    }
  }

  *out_request
      = (ti_http_request){ ._internal = {
                               .context = context,
                               .method = method,
                               .url = url,
                               .url_length = url_length,
                               /* query start is set to 0 if there is not a question mark so the
                                  next time query parameter is appended, a question mark will be
                                  added at url length. (+1 jumps the `?`) */
                               .query_start = query_start == url_length ? 0 : query_start + 1,
                               .headers = headers_buffer,
                               .headers_length = 0,
                               .max_headers = ti_span_size(headers_buffer)
                                   / (int32_t)sizeof(_ti_http_request_header),
                               .retry_headers_start_byte_offset = 0,
                               .body = body,
                           } };

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_request_set_query_parameter(
    ti_http_request* ref_request,
    ti_span name,
    ti_span value,
    bool is_value_url_encoded)
{
  _ti_PRECONDITION_NOT_NULL(ref_request);
  _ti_PRECONDITION_VALID_SPAN(name, 1, false);
  _ti_PRECONDITION_VALID_SPAN(value, 1, false);

  // name or value can't be empty
  _ti_PRECONDITION(ti_span_size(name) > 0 && ti_span_size(value) > 0);

  int32_t const initial_url_length = ref_request->_internal.url_length;
  ti_span url_remainder = ti_span_slice_to_end(ref_request->_internal.url, initial_url_length);

  // Adding query parameter. Adding +2 to required length to include extra required symbols `=`
  // and `?` or `&`.
  int32_t required_length = 2 + ti_span_size(name)
      + (is_value_url_encoded ? ti_span_size(value) : _ti_span_url_encode_calc_length(value));

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(url_remainder, required_length);

  // Append either '?' or '&'
  uint8_t separator = '&';
  if (ref_request->_internal.query_start == 0)
  {
    separator = '?';

    // update QPs starting position when it's 0
    ref_request->_internal.query_start = initial_url_length + 1;
  }

  url_remainder = ti_span_copy_u8(url_remainder, separator);
  url_remainder = ti_span_copy(url_remainder, name);

  // Append equal sym
  url_remainder = ti_span_copy_u8(url_remainder, '=');

  // Parameter value
  if (is_value_url_encoded)
  {
    ti_span_copy(url_remainder, value);
  }
  else
  {
    int32_t encoding_size = 0;
    _ti_RETURN_IF_FAILED(_ti_span_url_encode(url_remainder, value, &encoding_size));
  }

  ref_request->_internal.url_length += required_length;

  return TI_OK;
}

TI_NODISCARD ti_result
ti_http_request_append_header(ti_http_request* ref_request, ti_span name, ti_span value)
{
  _ti_PRECONDITION_NOT_NULL(ref_request);

  // remove whitespace characters from key and value
  name = _ti_span_trim_whitespace(name);
  value = _ti_span_trim_whitespace(value);

  _ti_PRECONDITION_VALID_SPAN(name, 1, false);

  // Make this function to only work with valid input for header name
  _ti_PRECONDITION(ti_http_is_valid_header_name(name));

  ti_span headers = ref_request->_internal.headers;
  _ti_http_request_header header_to_append = { .name = name, .value = value };

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(headers, (int32_t)sizeof header_to_append);

  ti_span_copy(
      ti_span_slice_to_end(
          headers,
          (int32_t)sizeof(_ti_http_request_header) * ref_request->_internal.headers_length),
      ti_span_create((uint8_t*)&header_to_append, sizeof header_to_append));

  ref_request->_internal.headers_length++;

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_request_get_header(
    ti_http_request const* request,
    int32_t index,
    ti_span* out_name,
    ti_span* out_value)
{
  _ti_PRECONDITION_NOT_NULL(request);
  _ti_PRECONDITION_NOT_NULL(out_name);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (index >= ti_http_request_headers_count(request))
  {
    return TI_ERROR_ARG;
  }

  _ti_http_request_header const* const header
      = &((_ti_http_request_header*)ti_span_ptr(request->_internal.headers))[index];

  *out_name = header->name;
  *out_value = header->value;

  return TI_OK;
}

TI_NODISCARD ti_result
ti_http_request_get_method(ti_http_request const* request, ti_http_method* out_method)
{
  _ti_PRECONDITION_NOT_NULL(request);
  _ti_PRECONDITION_NOT_NULL(out_method);

  *out_method = request->_internal.method;

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_request_get_url(ti_http_request const* request, ti_span* out_url)
{
  _ti_PRECONDITION_NOT_NULL(request);
  _ti_PRECONDITION_NOT_NULL(out_url);

  *out_url = ti_span_slice(request->_internal.url, 0, request->_internal.url_length);

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_request_get_body(ti_http_request const* request, ti_span* out_body)
{
  _ti_PRECONDITION_NOT_NULL(request);
  _ti_PRECONDITION_NOT_NULL(out_body);

  *out_body = request->_internal.body;
  return TI_OK;
}

TI_NODISCARD int32_t ti_http_request_headers_count(ti_http_request const* request)
{
  return request->_internal.headers_length;
}
