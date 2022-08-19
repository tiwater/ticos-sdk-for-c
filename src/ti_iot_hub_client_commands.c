// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_precondition.h>
#include <ti_result.h>
#include <ti_span.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_iot_hub_client.h>

#include <ti_log_internal.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg.h>

static const ti_span command_separator = TI_SPAN_LITERAL_FROM_STR("*");

TI_NODISCARD ti_result ti_iot_hub_client_commands_response_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return ti_iot_hub_client_methods_response_get_publish_topic(
      client, request_id, status, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
}

TI_NODISCARD ti_result ti_iot_hub_client_commands_parse_received_topic(
    ti_iot_hub_client const* client,
    ti_span received_topic,
    ti_iot_hub_client_command_request* out_request)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_request);

  ti_iot_hub_client_method_request method_request;

  _ti_RETURN_IF_FAILED(
      ti_iot_hub_client_methods_parse_received_topic(client, received_topic, &method_request));

  out_request->request_id = method_request.request_id;

  int32_t command_separator_index = ti_span_find(method_request.name, command_separator);
  if (command_separator_index > 0)
  {
    out_request->component_name = ti_span_slice(method_request.name, 0, command_separator_index);
    out_request->command_name = ti_span_slice(
        method_request.name, command_separator_index + 1, ti_span_size(method_request.name));
  }
  else
  {
    out_request->component_name = TI_SPAN_EMPTY;
    out_request->command_name
        = ti_span_slice(method_request.name, 0, ti_span_size(method_request.name));
  }

  return TI_OK;
}
