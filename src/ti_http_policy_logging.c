// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_http_policy_logging_private.h"
#include "ti_span_private.h"
#include <ti_http_transport.h>
#include <ti_platform.h>
#include <ti_http_internal.h>
#include <ti_log_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include <_ti_cfg.h>

enum
{
  _ti_LOG_LENGTHY_VALUE_MAX_LENGTH
  = 50, // When we print values, such as header values, if they are longer than
        // _ti_LOG_VALUE_MAX_LENGTH, we trim their contents (decorate with ellipsis in the middle)
        // to make sure each individual header value does not exceed _ti_LOG_VALUE_MAX_LENGTH so
        // that they don't blow up the logs.
};

static ti_span _ti_http_policy_logging_copy_lengthy_value(ti_span ref_log_msg, ti_span value)
{
  int32_t value_size = ti_span_size(value);

  // The caller should validate that ref_log_msg is large enough to contain the value ti_span
  // This means, ref_log_msg must have available at least _ti_LOG_LENGTHY_VALUE_MAX_LENGTH (i.e. 50)
  // bytes or as much as the size of the value ti_span, whichever is smaller.
  _ti_PRECONDITION(
      ti_span_size(ref_log_msg) >= _ti_LOG_LENGTHY_VALUE_MAX_LENGTH
      || ti_span_size(ref_log_msg) >= value_size);

  if (value_size <= _ti_LOG_LENGTHY_VALUE_MAX_LENGTH)
  {
    return ti_span_copy(ref_log_msg, value);
  }

  ti_span const ellipsis = TI_SPAN_FROM_STR(" ... ");
  int32_t const ellipsis_len = ti_span_size(ellipsis);

  int32_t const first
      = (_ti_LOG_LENGTHY_VALUE_MAX_LENGTH / 2) - ((ellipsis_len / 2) + (ellipsis_len % 2)); // 22

  int32_t const last
      = ((_ti_LOG_LENGTHY_VALUE_MAX_LENGTH / 2) + (_ti_LOG_LENGTHY_VALUE_MAX_LENGTH % 2)) // 23
      - (ellipsis_len / 2);

  _ti_PRECONDITION((first + last + ellipsis_len) == _ti_LOG_LENGTHY_VALUE_MAX_LENGTH);

  ref_log_msg = ti_span_copy(ref_log_msg, ti_span_slice(value, 0, first));
  ref_log_msg = ti_span_copy(ref_log_msg, ellipsis);
  return ti_span_copy(ref_log_msg, ti_span_slice(value, value_size - last, value_size));
}

static ti_result _ti_http_policy_logging_append_http_request_msg(
    ti_http_request const* request,
    ti_span* ref_log_msg)
{
  static ti_span const auth_header_name = TI_SPAN_LITERAL_FROM_STR("authorization");

  ti_span http_request_string = TI_SPAN_FROM_STR("HTTP Request : ");
  ti_span null_string = TI_SPAN_FROM_STR("NULL");

  int32_t required_length = ti_span_size(http_request_string);
  if (request == NULL)
  {
    required_length += ti_span_size(null_string);
  }
  else
  {
    required_length = ti_span_size(request->_internal.method) + request->_internal.url_length + 1;
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(*ref_log_msg, required_length);

  ti_span remainder = ti_span_copy(*ref_log_msg, http_request_string);

  if (request == NULL)
  {
    remainder = ti_span_copy(remainder, null_string);
    *ref_log_msg = ti_span_slice(*ref_log_msg, 0, _ti_span_diff(remainder, *ref_log_msg));
    return TI_OK;
  }

  remainder = ti_span_copy(remainder, request->_internal.method);
  remainder = ti_span_copy_u8(remainder, ' ');
  remainder = ti_span_copy(
      remainder, ti_span_slice(request->_internal.url, 0, request->_internal.url_length));

  int32_t const headers_count = ti_http_request_headers_count(request);

  ti_span new_line_tab_string = TI_SPAN_FROM_STR("\n\t");
  ti_span colon_separator_string = TI_SPAN_FROM_STR(" : ");

  for (int32_t index = 0; index < headers_count; ++index)
  {
    ti_span header_name = { 0 };
    ti_span header_value = { 0 };
    _ti_RETURN_IF_FAILED(ti_http_request_get_header(request, index, &header_name, &header_value));

    required_length = ti_span_size(new_line_tab_string) + ti_span_size(header_name);
    if (ti_span_size(header_value) > 0)
    {
      required_length += _ti_LOG_LENGTHY_VALUE_MAX_LENGTH + ti_span_size(colon_separator_string);
    }

    _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);
    remainder = ti_span_copy(remainder, new_line_tab_string);
    remainder = ti_span_copy(remainder, header_name);

    if (ti_span_size(header_value) > 0 && !ti_span_is_content_equal(header_name, auth_header_name))
    {
      remainder = ti_span_copy(remainder, colon_separator_string);
      remainder = _ti_http_policy_logging_copy_lengthy_value(remainder, header_value);
    }
  }
  *ref_log_msg = ti_span_slice(*ref_log_msg, 0, _ti_span_diff(remainder, *ref_log_msg));

  return TI_OK;
}

static ti_result _ti_http_policy_logging_append_http_response_msg(
    ti_http_response* ref_response,
    int64_t duration_msec,
    ti_http_request const* request,
    ti_span* ref_log_msg)
{
  ti_span http_response_string = TI_SPAN_FROM_STR("HTTP Response (");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(*ref_log_msg, ti_span_size(http_response_string));
  ti_span remainder = ti_span_copy(*ref_log_msg, http_response_string);

  _ti_RETURN_IF_FAILED(ti_span_i64toa(remainder, duration_msec, &remainder));

  ti_span ms_string = TI_SPAN_FROM_STR("ms)");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(ms_string));
  remainder = ti_span_copy(remainder, ms_string);

  if (ref_response == NULL || ti_span_size(ref_response->_internal.http_response) == 0)
  {
    ti_span is_empty_string = TI_SPAN_FROM_STR(" is empty");
    _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(is_empty_string));
    remainder = ti_span_copy(remainder, is_empty_string);

    *ref_log_msg = ti_span_slice(*ref_log_msg, 0, _ti_span_diff(remainder, *ref_log_msg));
    return TI_OK;
  }

  ti_span colon_separator_string = TI_SPAN_FROM_STR(" : ");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(colon_separator_string));
  remainder = ti_span_copy(remainder, colon_separator_string);

  ti_http_response_status_line status_line = { 0 };
  _ti_RETURN_IF_FAILED(ti_http_response_get_status_line(ref_response, &status_line));
  _ti_RETURN_IF_FAILED(ti_span_u64toa(remainder, (uint64_t)status_line.status_code, &remainder));

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(status_line.reason_phrase) + 1);
  remainder = ti_span_copy_u8(remainder, ' ');
  remainder = ti_span_copy(remainder, status_line.reason_phrase);

  ti_span new_line_tab_string = TI_SPAN_FROM_STR("\n\t");

  ti_result result = TI_OK;
  ti_span header_name = { 0 };
  ti_span header_value = { 0 };
  while (ti_result_succeeded(
      result = ti_http_response_get_next_header(ref_response, &header_name, &header_value)))
  {
    int32_t required_length = ti_span_size(new_line_tab_string) + ti_span_size(header_name);
    if (ti_span_size(header_value) > 0)
    {
      required_length += _ti_LOG_LENGTHY_VALUE_MAX_LENGTH + ti_span_size(colon_separator_string);
    }

    _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

    remainder = ti_span_copy(remainder, new_line_tab_string);
    remainder = ti_span_copy(remainder, header_name);

    if (ti_span_size(header_value) > 0)
    {
      remainder = ti_span_copy(remainder, colon_separator_string);
      remainder = _ti_http_policy_logging_copy_lengthy_value(remainder, header_value);
    }
  }

  // Response payload was invalid or corrupted in some way.
  if (result != TI_ERROR_HTTP_END_OF_HEADERS)
  {
    return result;
  }

  ti_span new_lines_string = TI_SPAN_FROM_STR("\n\n");
  ti_span arrow_separator_string = TI_SPAN_FROM_STR(" -> ");
  int32_t required_length = ti_span_size(new_lines_string) + ti_span_size(arrow_separator_string);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

  remainder = ti_span_copy(remainder, new_lines_string);
  remainder = ti_span_copy(remainder, arrow_separator_string);

  ti_span append_request = remainder;
  _ti_RETURN_IF_FAILED(_ti_http_policy_logging_append_http_request_msg(request, &append_request));

  *ref_log_msg = ti_span_slice(
      *ref_log_msg, 0, _ti_span_diff(remainder, *ref_log_msg) + ti_span_size(append_request));
  return TI_OK;
}

void _ti_http_policy_logging_log_http_request(ti_http_request const* request)
{
  uint8_t log_msg_buf[TI_LOG_MESSAGE_BUFFER_SIZE] = { 0 };
  ti_span log_msg = TI_SPAN_FROM_BUFFER(log_msg_buf);

  (void)_ti_http_policy_logging_append_http_request_msg(request, &log_msg);

  _ti_LOG_WRITE(TI_LOG_HTTP_REQUEST, log_msg);
}

void _ti_http_policy_logging_log_http_response(
    ti_http_response const* response,
    int64_t duration_msec,
    ti_http_request const* request)
{
  uint8_t log_msg_buf[TI_LOG_MESSAGE_BUFFER_SIZE] = { 0 };
  ti_span log_msg = TI_SPAN_FROM_BUFFER(log_msg_buf);

  ti_http_response response_copy = *response;

  (void)_ti_http_policy_logging_append_http_response_msg(
      &response_copy, duration_msec, request, &log_msg);

  _ti_LOG_WRITE(TI_LOG_HTTP_RESPONSE, log_msg);
}

#ifndef TI_NO_LOGGING
TI_NODISCARD ti_result ti_http_pipeline_policy_logging(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  (void)ref_options;

  if (_ti_LOG_SHOULD_WRITE(TI_LOG_HTTP_REQUEST))
  {
    _ti_http_policy_logging_log_http_request(ref_request);
  }

  if (!_ti_LOG_SHOULD_WRITE(TI_LOG_HTTP_RESPONSE))
  {
    // If no logging is needed, do not even measure the response time.
    return _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
  }

  int64_t start = 0;
  _ti_RETURN_IF_FAILED(ti_platform_clock_msec(&start));

  ti_result const result = _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);

  int64_t end = 0;
  _ti_RETURN_IF_FAILED(ti_platform_clock_msec(&end));
  _ti_http_policy_logging_log_http_response(ref_response, end - start, ref_request);

  return result;
}
#endif // TI_NO_LOGGING
