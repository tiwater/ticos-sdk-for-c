// Copyright (c) Tiwater Technologies. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_result.h>
#include <ti_span.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>
#include <ti_iot_hub_client.h>

#include <ti_log_internal.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg.h>

static const ti_span c2d_topic_suffix = TI_SPAN_LITERAL_FROM_STR("/commands/request/");

TI_NODISCARD ti_result ti_iot_hub_client_c2d_parse_received_topic(
    ti_iot_hub_client const* client,
    ti_span received_topic,
    ti_iot_hub_client_c2d_request* out_request)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(client->_internal.iot_hub_hostname, 1, false);
  _ti_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_request);
  (void)client;

  int32_t index = 0;
  ti_span remainder;
  (void)_ti_span_token(received_topic, c2d_topic_suffix, &remainder, &index);
  if (index == -1)
  {
    return TI_ERROR_IOT_TOPIC_NO_MATCH;
  }

  if (_ti_LOG_SHOULD_WRITE(TI_LOG_MQTT_RECEIVED_TOPIC))
  {
    _ti_LOG_WRITE(TI_LOG_MQTT_RECEIVED_TOPIC, received_topic);
  }

  ti_span token = ti_span_size(remainder) == 0
      ? TI_SPAN_EMPTY
      : _ti_span_token(remainder, c2d_topic_suffix, &remainder, &index);

  _ti_RETURN_IF_FAILED(
      ti_iot_message_properties_init(&out_request->properties, token, ti_span_size(token)));

  return TI_OK;
}
