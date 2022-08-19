// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_result.h>
#include <ti_span.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>
#include <ti_iot_common.h>
#include <ti_iot_common_internal.h>

#include <ti_log_internal.h>
#include <ti_retry_internal.h>

#include <_ti_cfg.h>

static const ti_span hub_client_param_separator_span = TI_SPAN_LITERAL_FROM_STR("&");
static const ti_span hub_client_param_equals_span = TI_SPAN_LITERAL_FROM_STR("=");

TI_NODISCARD ti_result ti_iot_message_properties_init(
    ti_iot_message_properties* properties,
    ti_span buffer,
    int32_t written_length)
{
  _ti_PRECONDITION_NOT_NULL(properties);
  _ti_PRECONDITION_VALID_SPAN(buffer, 0, true);
  _ti_PRECONDITION_RANGE(0, written_length, ti_span_size(buffer));

  properties->_internal.properties_buffer = buffer;
  properties->_internal.properties_written = written_length;
  properties->_internal.current_property_index = 0;

  return TI_OK;
}

TI_NODISCARD ti_result
ti_iot_message_properties_append(ti_iot_message_properties* properties, ti_span name, ti_span value)
{
  _ti_PRECONDITION_NOT_NULL(properties);
  _ti_PRECONDITION_VALID_SPAN(name, 1, false);
  _ti_PRECONDITION_VALID_SPAN(value, 1, false);

  int32_t prop_length = properties->_internal.properties_written;

  ti_span remainder = ti_span_slice_to_end(properties->_internal.properties_buffer, prop_length);

  int32_t required_length = ti_span_size(name) + ti_span_size(value) + 1;

  if (prop_length > 0)
  {
    required_length += 1;
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, required_length);

  if (prop_length > 0)
  {
    remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_separator_span));
  }

  remainder = ti_span_copy(remainder, name);
  remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_equals_span));
  ti_span_copy(remainder, value);

  properties->_internal.properties_written += required_length;

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_message_properties_find(
    ti_iot_message_properties* properties,
    ti_span name,
    ti_span* out_value)
{
  _ti_PRECONDITION_NOT_NULL(properties);
  _ti_PRECONDITION_VALID_SPAN(name, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_value);

  ti_span remaining = ti_span_slice(
      properties->_internal.properties_buffer, 0, properties->_internal.properties_written);

  while (ti_span_size(remaining) != 0)
  {
    int32_t index = 0;
    ti_span delim_span
        = _ti_span_token(remaining, hub_client_param_equals_span, &remaining, &index);
    if (index != -1)
    {
      if (ti_span_is_content_equal(delim_span, name))
      {
        *out_value = _ti_span_token(remaining, hub_client_param_separator_span, &remaining, &index);
        return TI_OK;
      }

      _ti_span_token(remaining, hub_client_param_separator_span, &remaining, &index);
    }
  }

  return TI_ERROR_ITEM_NOT_FOUND;
}

TI_NODISCARD ti_result ti_iot_message_properties_next(
    ti_iot_message_properties* properties,
    ti_span* out_name,
    ti_span* out_value)
{
  _ti_PRECONDITION_NOT_NULL(properties);
  _ti_PRECONDITION_NOT_NULL(out_name);
  _ti_PRECONDITION_NOT_NULL(out_value);

  int32_t index = (int32_t)properties->_internal.current_property_index;
  int32_t prop_length = properties->_internal.properties_written;

  if (index == prop_length)
  {
    *out_name = TI_SPAN_EMPTY;
    *out_value = TI_SPAN_EMPTY;
    return TI_ERROR_IOT_END_OF_PROPERTIES;
  }

  ti_span remainder;
  ti_span prop_span = ti_span_slice(properties->_internal.properties_buffer, index, prop_length);

  int32_t location = 0;
  *out_name = _ti_span_token(prop_span, hub_client_param_equals_span, &remainder, &location);
  *out_value = _ti_span_token(remainder, hub_client_param_separator_span, &remainder, &location);
  if (ti_span_size(remainder) == 0)
  {
    properties->_internal.current_property_index = (uint32_t)prop_length;
  }
  else
  {
    properties->_internal.current_property_index += (uint32_t)(_ti_span_diff(remainder, prop_span));
  }

  return TI_OK;
}

TI_NODISCARD int32_t ti_iot_calculate_retry_delay(
    int32_t operation_msec,
    int16_t attempt,
    int32_t min_retry_delay_msec,
    int32_t max_retry_delay_msec,
    int32_t random_jitter_msec)
{
  _ti_PRECONDITION_RANGE(0, operation_msec, INT32_MAX - 1);
  _ti_PRECONDITION_RANGE(0, attempt, INT16_MAX - 1);
  _ti_PRECONDITION_RANGE(0, min_retry_delay_msec, INT32_MAX - 1);
  _ti_PRECONDITION_RANGE(0, max_retry_delay_msec, INT32_MAX - 1);
  _ti_PRECONDITION_RANGE(0, random_jitter_msec, INT32_MAX - 1);

  if (_ti_LOG_SHOULD_WRITE(TI_LOG_IOT_RETRY))
  {
    _ti_LOG_WRITE(TI_LOG_IOT_RETRY, TI_SPAN_EMPTY);
  }

  int32_t delay = _ti_retry_calc_delay(attempt, min_retry_delay_msec, max_retry_delay_msec);

  if (delay < 0)
  {
    delay = max_retry_delay_msec;
  }

  if (max_retry_delay_msec - delay > random_jitter_msec)
  {
    delay += random_jitter_msec;
  }

  delay -= operation_msec;

  return delay > 0 ? delay : 0;
}

TI_NODISCARD int32_t _ti_iot_u32toa_size(uint32_t number)
{
  if (number == 0)
  {
    return 1;
  }

  uint32_t div = _ti_SMALLEST_10_DIGIT_NUMBER;
  int32_t digit_count = _ti_MAX_SIZE_FOR_UINT32;
  while (number / div == 0)
  {
    div /= _ti_NUMBER_OF_DECIMAL_VALUES;
    digit_count--;
  }

  return digit_count;
}

TI_NODISCARD int32_t _ti_iot_u64toa_size(uint64_t number)
{
  if (number == 0)
  {
    return 1;
  }

  uint64_t div = _ti_SMALLEST_20_DIGIT_NUMBER;
  int32_t digit_count = _ti_MAX_SIZE_FOR_UINT64;
  while (number / div == 0)
  {
    div /= _ti_NUMBER_OF_DECIMAL_VALUES;
    digit_count--;
  }

  return digit_count;
}

TI_NODISCARD ti_result
_ti_span_copy_url_encode(ti_span destination, ti_span source, ti_span* out_remainder)
{
  int32_t length = 0;
  _ti_RETURN_IF_FAILED(_ti_span_url_encode(destination, source, &length));
  *out_remainder = ti_span_slice(destination, length, ti_span_size(destination));
  return TI_OK;
}
