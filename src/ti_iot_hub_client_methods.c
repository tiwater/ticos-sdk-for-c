// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_result.h>
#include <ti_span.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>
#include <ti_iot_hub_client.h>
#include <ti_iot_common_internal.h>

#include <ti_log_internal.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg.h>

static const uint8_t null_terminator = '\0';
static const ti_span methods_topic_prefix = TI_SPAN_LITERAL_FROM_STR("$iothub/methods/");
static const ti_span methods_topic_filter_suffix = TI_SPAN_LITERAL_FROM_STR("POST/");
static const ti_span methods_response_topic_result = TI_SPAN_LITERAL_FROM_STR("res/");
static const ti_span methods_response_topic_properties = TI_SPAN_LITERAL_FROM_STR("/?$rid=");

TI_NODISCARD ti_result ti_iot_hub_client_methods_parse_received_topic(
    ti_iot_hub_client const* client,
    ti_span received_topic,
    ti_iot_hub_client_method_request* out_request)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(client->_internal.iot_hub_hostname, 1, false);
  _ti_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_request);

  (void)client;

  int32_t index = ti_span_find(received_topic, methods_topic_prefix);

  if (index == -1)
  {
    return TI_ERROR_IOT_TOPIC_NO_MATCH;
  }

  if (_ti_LOG_SHOULD_WRITE(TI_LOG_MQTT_RECEIVED_TOPIC))
  {
    _ti_LOG_WRITE(TI_LOG_MQTT_RECEIVED_TOPIC, received_topic);
  }

  received_topic = ti_span_slice(
      received_topic, index + ti_span_size(methods_topic_prefix), ti_span_size(received_topic));

  index = ti_span_find(received_topic, methods_topic_filter_suffix);

  if (index == -1)
  {
    return TI_ERROR_IOT_TOPIC_NO_MATCH;
  }

  received_topic = ti_span_slice(
      received_topic,
      index + ti_span_size(methods_topic_filter_suffix),
      ti_span_size(received_topic));

  index = ti_span_find(received_topic, methods_response_topic_properties);

  if (index == -1)
  {
    return TI_ERROR_IOT_TOPIC_NO_MATCH;
  }

  out_request->name = ti_span_slice(received_topic, 0, index);
  out_request->request_id = ti_span_slice(
      received_topic,
      index + ti_span_size(methods_response_topic_properties),
      ti_span_size(received_topic));

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_methods_response_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(client->_internal.iot_hub_hostname, 1, false);
  _ti_PRECONDITION_VALID_SPAN(request_id, 1, false);
  _ti_PRECONDITION_NOT_NULL(mqtt_topic);
  _ti_PRECONDITION(mqtt_topic_size > 0);

  (void)client;

  ti_span mqtt_topic_span = ti_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  int32_t required_length = ti_span_size(methods_topic_prefix)
      + ti_span_size(methods_response_topic_result) + _ti_iot_u32toa_size(status)
      + ti_span_size(methods_response_topic_properties) + ti_span_size(request_id);

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_topic_span, methods_topic_prefix);
  remainder = ti_span_copy(remainder, methods_response_topic_result);

  _ti_RETURN_IF_FAILED(ti_span_u32toa(remainder, (uint32_t)status, &remainder));

  remainder = ti_span_copy(remainder, methods_response_topic_properties);
  remainder = ti_span_copy(remainder, request_id);
  ti_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return TI_OK;
}
