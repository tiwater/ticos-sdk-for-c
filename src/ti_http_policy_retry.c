// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_http_private.h"
#include <ti_config.h>
#include <ti_platform.h>
#include <ti_config_internal.h>
#include <ti_http_internal.h>
#include <ti_log_internal.h>
#include <ti_result_internal.h>
#include <ti_retry_internal.h>
#include <ti_span_internal.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_http_policy_retry_options _ti_http_policy_retry_options_default()
{
  return (ti_http_policy_retry_options){
    .max_retries = 4,
    .retry_delay_msec = 4 * _ti_TIME_MILLISECONDS_PER_SECOND, // 4 seconds
    .max_retry_delay_msec
    = 2 * _ti_TIME_SECONDS_PER_MINUTE * _ti_TIME_MILLISECONDS_PER_SECOND, // 2 minutes
  };
}

// TODO: Add unit tests
TI_INLINE ti_result _ti_http_policy_retry_append_http_retry_msg(
    int32_t attempt,
    int32_t delay_msec,
    ti_span* ref_log_msg)
{
  ti_span retry_count_string = TI_SPAN_FROM_STR("HTTP Retry attempt #");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(*ref_log_msg, ti_span_size(retry_count_string));
  ti_span remainder = ti_span_copy(*ref_log_msg, retry_count_string);

  _ti_RETURN_IF_FAILED(ti_span_i32toa(remainder, attempt, &remainder));

  ti_span infix_string = TI_SPAN_FROM_STR(" will be made in ");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(infix_string));
  remainder = ti_span_copy(remainder, infix_string);

  _ti_RETURN_IF_FAILED(ti_span_i32toa(remainder, delay_msec, &remainder));

  ti_span suffix_string = TI_SPAN_FROM_STR("ms.");
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(suffix_string));
  remainder = ti_span_copy(remainder, suffix_string);

  *ref_log_msg = ti_span_slice(*ref_log_msg, 0, _ti_span_diff(remainder, *ref_log_msg));

  return TI_OK;
}

TI_INLINE void _ti_http_policy_retry_log(int32_t attempt, int32_t delay_msec)
{
  uint8_t log_msg_buf[TI_LOG_MESSAGE_BUFFER_SIZE] = { 0 };
  ti_span log_msg = TI_SPAN_FROM_BUFFER(log_msg_buf);

  (void)_ti_http_policy_retry_append_http_retry_msg(attempt, delay_msec, &log_msg);

  _ti_LOG_WRITE(TI_LOG_HTTP_RETRY, log_msg);
}

TI_INLINE TI_NODISCARD int32_t _ti_uint32_span_to_int32(ti_span span)
{
  uint32_t value = 0;
  if (ti_result_failed(ti_span_atou32(span, &value)))
  {
    return -1;
  }

  return value < INT32_MAX ? (int32_t)value : INT32_MAX;
}

TI_INLINE TI_NODISCARD bool _ti_http_policy_retry_should_retry_http_response_code(
    ti_http_status_code http_response_code)
{
  switch (http_response_code)
  {
    case TI_HTTP_STATUS_CODE_REQUEST_TIMEOUT:
    case TI_HTTP_STATUS_CODE_TOO_MANY_REQUESTS:
    case TI_HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR:
    case TI_HTTP_STATUS_CODE_BAD_GATEWAY:
    case TI_HTTP_STATUS_CODE_SERVICE_UNAVAILABLE:
    case TI_HTTP_STATUS_CODE_GATEWAY_TIMEOUT:
      return true;
    default:
      return false;
  }
}

TI_INLINE TI_NODISCARD ti_result _ti_http_policy_retry_get_retry_after(
    ti_http_response* ref_response,
    bool* should_retry,
    int32_t* retry_after_msec)
{
  ti_http_response_status_line status_line = { 0 };
  _ti_RETURN_IF_FAILED(ti_http_response_get_status_line(ref_response, &status_line));

  if (!_ti_http_policy_retry_should_retry_http_response_code(status_line.status_code))
  {
    *should_retry = false;
    *retry_after_msec = -1;
    return TI_OK;
  }

  *should_retry = true;

  // Try to get the value of retry-after header, if there's one.
  ti_span header_name = { 0 };
  ti_span header_value = { 0 };
  while (ti_result_succeeded(
      ti_http_response_get_next_header(ref_response, &header_name, &header_value)))
  {
    if (ti_span_is_content_equal_ignoring_case(header_name, TI_SPAN_FROM_STR("retry-after-ms"))
        || ti_span_is_content_equal_ignoring_case(
            header_name, TI_SPAN_FROM_STR("x-ms-retry-after-ms")))
    {
      // The value is in milliseconds.
      int32_t const msec = _ti_uint32_span_to_int32(header_value);
      if (msec >= 0) // int32_t max == ~24 days
      {
        *retry_after_msec = msec;
        return TI_OK;
      }
    }
    else if (ti_span_is_content_equal_ignoring_case(header_name, TI_SPAN_FROM_STR("Retry-After")))
    {
      // The value is either seconds or date.
      int32_t const seconds = _ti_uint32_span_to_int32(header_value);
      if (seconds >= 0) // int32_t max == ~68 years
      {
        *retry_after_msec = (seconds <= (INT32_MAX / _ti_TIME_MILLISECONDS_PER_SECOND))
            ? seconds * _ti_TIME_MILLISECONDS_PER_SECOND
            : INT32_MAX;

        return TI_OK;
      }

      // TODO: Other possible value is HTTP Date. For that, we'll need to parse date, get
      // current date, subtract one from another, get seconds. And the device should have a
      // sense of calendar clock.
    }
  }

  *retry_after_msec = -1;
  return TI_OK;
}

TI_NODISCARD ti_result ti_http_pipeline_policy_retry(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  ti_http_policy_retry_options const* const retry_options
      = (ti_http_policy_retry_options const*)ref_options;

  int32_t const max_retries = retry_options->max_retries;
  int32_t const retry_delay_msec = retry_options->retry_delay_msec;
  int32_t const max_retry_delay_msec = retry_options->max_retry_delay_msec;

  _ti_RETURN_IF_FAILED(_ti_http_request_mark_retry_headers_start(ref_request));

  ti_context* const context = ref_request->_internal.context;

  bool const should_log = _ti_LOG_SHOULD_WRITE(TI_LOG_HTTP_RETRY);
  ti_result result = TI_OK;
  int32_t attempt = 1;
  while (true)
  {
    _ti_RETURN_IF_FAILED(
        ti_http_response_init(ref_response, ref_response->_internal.http_response));
    _ti_RETURN_IF_FAILED(_ti_http_request_remove_retry_headers(ref_request));

    result = _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);

    // Even HTTP 429, or 502 are expected to be TI_OK, so the failed result is not retriable.
    if (attempt > max_retries || ti_result_failed(result))
    {
      return result;
    }

    int32_t retry_after_msec = -1;
    bool should_retry = false;
    ti_http_response response_copy = *ref_response;

    _ti_RETURN_IF_FAILED(
        _ti_http_policy_retry_get_retry_after(&response_copy, &should_retry, &retry_after_msec));

    if (!should_retry)
    {
      return result;
    }

    ++attempt;

    if (retry_after_msec < 0)
    { // there wasn't any kind of "retry-after" response header
      retry_after_msec = _ti_retry_calc_delay(attempt, retry_delay_msec, max_retry_delay_msec);
    }

    if (should_log)
    {
      _ti_http_policy_retry_log(attempt, retry_after_msec);
    }

    _ti_RETURN_IF_FAILED(ti_platform_sleep_msec(retry_after_msec));

    if (context != NULL)
    {
      int64_t clock = 0;
      _ti_RETURN_IF_FAILED(ti_platform_clock_msec(&clock));
      if (ti_context_has_expired(context, clock))
      {
        return TI_ERROR_CANCELED;
      }
    }
  }
}
