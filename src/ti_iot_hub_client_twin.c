// Copyright (c) Tiwater Technologies. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_precondition.h>
#include <ti_result.h>
#include <ti_span.h>
#include <ti_log_internal.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>
#include <ti_iot_hub_client.h>

#include <_ti_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t ti_iot_hub_client_twin_question = '?';
static const uint8_t ti_iot_hub_client_twin_equals = '=';
static const ti_span ti_iot_hub_client_request_id_span = TI_SPAN_LITERAL_FROM_STR("$rid");
static const ti_span ti_iot_hub_twin_topic_prefix = TI_SPAN_LITERAL_FROM_STR("devices/");
static const ti_span ti_iot_hub_twin_response_sub_topic = TI_SPAN_LITERAL_FROM_STR("res/");
static const ti_span ti_iot_hub_twin_get_pub_topic = TI_SPAN_LITERAL_FROM_STR("get/");
static const ti_span ti_iot_hub_twin_version_prop = TI_SPAN_LITERAL_FROM_STR("$version");
static const ti_span ti_iot_hub_twin_patch_pub_topic
    = TI_SPAN_LITERAL_FROM_STR("/twin/patch/reported/");
static const ti_span ti_iot_hub_twin_patch_sub_topic
    = TI_SPAN_LITERAL_FROM_STR("/twin/patch/desired/");

TI_NODISCARD ti_result ti_iot_hub_client_twin_document_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
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
  int32_t required_length = ti_span_size(ti_iot_hub_twin_topic_prefix)
      + ti_span_size(ti_iot_hub_twin_get_pub_topic)
      + (int32_t)sizeof(ti_iot_hub_client_twin_question)
      + ti_span_size(ti_iot_hub_client_request_id_span)
      + (int32_t)sizeof(ti_iot_hub_client_twin_equals) + ti_span_size(request_id);

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_topic_span, ti_iot_hub_twin_topic_prefix);
  remainder = ti_span_copy(remainder, ti_iot_hub_twin_get_pub_topic);
  remainder = ti_span_copy_u8(remainder, ti_iot_hub_client_twin_question);
  remainder = ti_span_copy(remainder, ti_iot_hub_client_request_id_span);
  remainder = ti_span_copy_u8(remainder, ti_iot_hub_client_twin_equals);
  remainder = ti_span_copy(remainder, request_id);
  ti_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_twin_patch_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
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
  int32_t required_length = ti_span_size(ti_iot_hub_twin_topic_prefix)
      + ti_span_size(client->_internal.device_id)
      + ti_span_size(ti_iot_hub_twin_patch_pub_topic)
      + (int32_t)sizeof(ti_iot_hub_client_twin_question)
      + ti_span_size(ti_iot_hub_client_request_id_span)
      + (int32_t)sizeof(ti_iot_hub_client_twin_equals) + ti_span_size(request_id);

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_topic_span, ti_iot_hub_twin_topic_prefix);
  remainder = ti_span_copy(remainder, client->_internal.device_id);
  remainder = ti_span_copy(remainder, ti_iot_hub_twin_patch_pub_topic);
  remainder = ti_span_copy_u8(remainder, ti_iot_hub_client_twin_question);
  remainder = ti_span_copy(remainder, ti_iot_hub_client_request_id_span);
  remainder = ti_span_copy_u8(remainder, ti_iot_hub_client_twin_equals);
  remainder = ti_span_copy(remainder, request_id);
  ti_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_twin_parse_received_topic(
    ti_iot_hub_client const* client,
    ti_span received_topic,
    ti_iot_hub_client_twin_response* out_response)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(client->_internal.iot_hub_hostname, 1, false);
  _ti_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_response);
  (void)client;

  ti_result result = TI_OK;

  int32_t twin_index = ti_span_find(received_topic, ti_iot_hub_twin_topic_prefix);
  // Check if is related to twin or not
  if (twin_index >= 0)
  {
    _ti_LOG_WRITE(TI_LOG_MQTT_RECEIVED_TOPIC, received_topic);

    int32_t twin_feature_index = -1;
    ti_span twin_feature_span
        = ti_span_slice(received_topic, twin_index, ti_span_size(received_topic));

    if ((twin_feature_index = ti_span_find(twin_feature_span, ti_iot_hub_twin_response_sub_topic))
        >= 0)
    {
      // Is a res case
      int32_t index = 0;
      ti_span remainder;
      ti_span status_str = _ti_span_token(
          ti_span_slice(
              received_topic,
              twin_feature_index + ti_span_size(ti_iot_hub_twin_response_sub_topic),
              ti_span_size(received_topic)),
          TI_SPAN_FROM_STR("/"),
          &remainder,
          &index);

      // Get status and convert to enum
      uint32_t status_int = 0;
      _ti_RETURN_IF_FAILED(ti_span_atou32(status_str, &status_int));
      out_response->status = (ti_iot_status)status_int;

      if (index == -1)
      {
        return TI_ERROR_UNEXPECTED_END;
      }

      // Get request id prop value
      ti_iot_message_properties props;
      ti_span prop_span = ti_span_slice(remainder, 1, ti_span_size(remainder));
      _ti_RETURN_IF_FAILED(
          ti_iot_message_properties_init(&props, prop_span, ti_span_size(prop_span)));
      _ti_RETURN_IF_FAILED(ti_iot_message_properties_find(
          &props, ti_iot_hub_client_request_id_span, &out_response->request_id));

      if (out_response->status >= TI_IOT_STATUS_BAD_REQUEST) // 400+
      {
        // Is an error response
        out_response->response_type = TI_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REQUEST_ERROR;
        out_response->version = TI_SPAN_EMPTY;
      }
      else if (out_response->status == TI_IOT_STATUS_NO_CONTENT) // 204
      {
        // Is a reported prop response
        out_response->response_type = TI_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES;

        result = ti_iot_message_properties_find(
            &props, ti_iot_hub_twin_version_prop, &out_response->version);
        if (result == TI_ERROR_ITEM_NOT_FOUND)
        {
          out_response->version = TI_SPAN_EMPTY;
        }
        else
        {
          _ti_RETURN_IF_FAILED(result);
        }
      }
      else // 200 or 202
      {
        // Is a twin GET response
        out_response->response_type = TI_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_GET;
        out_response->version = TI_SPAN_EMPTY;
      }

      result = TI_OK;
    }
    else if (
        (twin_feature_index = ti_span_find(twin_feature_span, ti_iot_hub_twin_patch_sub_topic))
        >= 0)
    {
      // Is a /PATCH case (desired props)
      ti_iot_message_properties props;
      ti_span prop_span = ti_span_slice(
          received_topic,
          twin_feature_index + ti_span_size(ti_iot_hub_twin_patch_sub_topic)
              + (int32_t)sizeof(ti_iot_hub_client_twin_question),
          ti_span_size(received_topic));
      _ti_RETURN_IF_FAILED(
          ti_iot_message_properties_init(&props, prop_span, ti_span_size(prop_span)));
      _ti_RETURN_IF_FAILED(ti_iot_message_properties_find(
          &props, ti_iot_hub_twin_version_prop, &out_response->version));

      out_response->response_type = TI_IOT_HUB_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES;
      out_response->request_id = TI_SPAN_EMPTY;
      out_response->status = TI_IOT_STATUS_OK;

      result = TI_OK;
    }
    else
    {
      result = TI_ERROR_IOT_TOPIC_NO_MATCH;
    }
  }
  else
  {
    result = TI_ERROR_IOT_TOPIC_NO_MATCH;
  }

  return result;
}
