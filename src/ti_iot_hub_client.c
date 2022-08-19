// Copyright (c) Tiwater Technologies. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <ti_result.h>
#include <ti_span.h>
#include <ti_version.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>
#include <ti_iot_hub_client.h>
#include <ti_iot_common_internal.h>

#include <_ti_cfg.h>

static const uint8_t null_terminator = '\0';
static const uint8_t hub_client_forward_slash = '/';
static const ti_span hub_client_param_separator_span = TI_SPAN_LITERAL_FROM_STR("&");
static const ti_span hub_client_param_equals_span = TI_SPAN_LITERAL_FROM_STR("=");

static const ti_span hub_digital_twin_model_id = TI_SPAN_LITERAL_FROM_STR("model-id");
static const ti_span hub_service_api_version = TI_SPAN_LITERAL_FROM_STR("/?api-version=2022-06-06");
static const ti_span client_sdk_device_client_type_name
    = TI_SPAN_LITERAL_FROM_STR("DeviceClientType");
static const ti_span client_sdk_version_default_value
    = TI_SPAN_LITERAL_FROM_STR("tisdk-c%2F" TI_SDK_VERSION_STRING);

TI_NODISCARD ti_iot_hub_client_options ti_iot_hub_client_options_default()
{
  return (ti_iot_hub_client_options){ .module_id = TI_SPAN_EMPTY,
                                      .user_agent = client_sdk_version_default_value,
                                      .model_id = TI_SPAN_EMPTY,
                                      .component_names = NULL,
                                      .component_names_length = 0 };
}

TI_NODISCARD ti_result ti_iot_hub_client_init(
    ti_iot_hub_client* client,
    ti_span iot_hub_hostname,
    ti_span device_id,
    ti_iot_hub_client_options const* options)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(iot_hub_hostname, 1, false);
  _ti_PRECONDITION_VALID_SPAN(device_id, 1, false);

  client->_internal.iot_hub_hostname = iot_hub_hostname;
  client->_internal.device_id = device_id;
  client->_internal.options = options == NULL ? ti_iot_hub_client_options_default() : *options;

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_get_user_name(
    ti_iot_hub_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(mqtt_user_name);
  _ti_PRECONDITION(mqtt_user_name_size > 0);

  const ti_span* const module_id = &(client->_internal.options.module_id);
  const ti_span* const user_agent = &(client->_internal.options.user_agent);
  const ti_span* const model_id = &(client->_internal.options.model_id);

  ti_span mqtt_user_name_span
      = ti_span_create((uint8_t*)mqtt_user_name, (int32_t)mqtt_user_name_size);

  int32_t required_length = ti_span_size(client->_internal.iot_hub_hostname) + (int32_t)sizeof(hub_client_forward_slash)
      + ti_span_size(client->_internal.device_id) + (int32_t)sizeof(hub_client_forward_slash)
      + ti_span_size(hub_service_api_version);
  if (ti_span_size(*module_id) > 0)
  {
    required_length += ti_span_size(*module_id) + (int32_t)sizeof(hub_client_forward_slash);
  }
  if (ti_span_size(*user_agent) > 0)
  {
    required_length += ti_span_size(hub_client_param_separator_span)
        + ti_span_size(client_sdk_device_client_type_name)
        + ti_span_size(hub_client_param_equals_span) + ti_span_size(*user_agent);
  }
  // Note we skip the length of the model id since we have to url encode it. Bound checking is done
  // later.
  if (ti_span_size(*model_id) > 0)
  {
    required_length += ti_span_size(hub_client_param_separator_span)
        + ti_span_size(hub_client_param_equals_span);
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_user_name_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_user_name_span, client->_internal.iot_hub_hostname);
  remainder = ti_span_copy_u8(remainder, hub_client_forward_slash);
  remainder = ti_span_copy(remainder, client->_internal.device_id);

  if (ti_span_size(*module_id) > 0)
  {
    remainder = ti_span_copy_u8(remainder, hub_client_forward_slash);
    remainder = ti_span_copy(remainder, *module_id);
  }

  remainder = ti_span_copy(remainder, hub_service_api_version);

  if (ti_span_size(*user_agent) > 0)
  {
    remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_separator_span));
    remainder = ti_span_copy(remainder, client_sdk_device_client_type_name);
    remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_equals_span));
    remainder = ti_span_copy(remainder, *user_agent);
  }

  if (ti_span_size(*model_id) > 0)
  {
    remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_separator_span));
    remainder = ti_span_copy(remainder, hub_digital_twin_model_id);
    remainder = ti_span_copy_u8(remainder, *ti_span_ptr(hub_client_param_equals_span));

    _ti_RETURN_IF_FAILED(_ti_span_copy_url_encode(remainder, *model_id, &remainder));
  }
  if (ti_span_size(remainder) > 0)
  {
    remainder = ti_span_copy_u8(remainder, null_terminator);
  }
  else
  {
    return TI_ERROR_NOT_ENOUGH_SPACE;
  }

  if (out_mqtt_user_name_length)
  {
    *out_mqtt_user_name_length
        = mqtt_user_name_size - (size_t)ti_span_size(remainder) - sizeof(null_terminator);
  }

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_get_client_id(
    ti_iot_hub_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(mqtt_client_id);
  _ti_PRECONDITION(mqtt_client_id_size > 0);

  ti_span mqtt_client_id_span
      = ti_span_create((uint8_t*)mqtt_client_id, (int32_t)mqtt_client_id_size);
  const ti_span* const module_id = &(client->_internal.options.module_id);

  int32_t required_length = ti_span_size(client->_internal.device_id);
  if (ti_span_size(*module_id) > 0)
  {
    required_length += ti_span_size(*module_id) + (int32_t)sizeof(hub_client_forward_slash);
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_client_id_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_client_id_span, client->_internal.device_id);

  if (ti_span_size(*module_id) > 0)
  {
    remainder = ti_span_copy_u8(remainder, hub_client_forward_slash);
    remainder = ti_span_copy(remainder, *module_id);
  }

  ti_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_client_id_length)
  {
    *out_mqtt_client_id_length = (size_t)required_length;
  }

  return TI_OK;
}
