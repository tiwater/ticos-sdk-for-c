// Copyright (c) Tiwater Technologies. All rights reserved.
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_precondition.h>
#include <ti_result.h>
#include <ti_span.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_iot_hub_client.h>

#include <stdint.h>

#include <_ti_cfg.h>

static const uint8_t null_terminator = '\0';
static const ti_span telemetry_topic_prefix = TI_SPAN_LITERAL_FROM_STR("devices/");
static const ti_span telemetry_topic_modules_mid = TI_SPAN_LITERAL_FROM_STR("/modules/");
static const ti_span telemetry_topic_suffix = TI_SPAN_LITERAL_FROM_STR("/telemetry");

TI_NODISCARD ti_result ti_iot_hub_client_telemetry_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_iot_message_properties const* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(mqtt_topic);
  _ti_PRECONDITION(mqtt_topic_size > 0);

  const ti_span* const module_id = &(client->_internal.options.module_id);

  ti_span mqtt_topic_span = ti_span_create((uint8_t*)mqtt_topic, (int32_t)mqtt_topic_size);
  int32_t required_length = ti_span_size(telemetry_topic_prefix)
      + ti_span_size(client->_internal.device_id) + ti_span_size(telemetry_topic_suffix);
  int32_t module_id_length = ti_span_size(*module_id);
  if (module_id_length > 0)
  {
    required_length += ti_span_size(telemetry_topic_modules_mid) + module_id_length;
  }
  if (properties != NULL)
  {
    required_length += properties->_internal.properties_written;
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_topic_span, required_length + (int32_t)sizeof(null_terminator));

  ti_span remainder = ti_span_copy(mqtt_topic_span, telemetry_topic_prefix);
  remainder = ti_span_copy(remainder, client->_internal.device_id);

  if (module_id_length > 0)
  {
    remainder = ti_span_copy(remainder, telemetry_topic_modules_mid);
    remainder = ti_span_copy(remainder, *module_id);
  }

  remainder = ti_span_copy(remainder, telemetry_topic_suffix);

  if (properties != NULL)
  {
    remainder = ti_span_copy(
        remainder,
        ti_span_slice(
            properties->_internal.properties_buffer, 0, properties->_internal.properties_written));
  }

  ti_span_copy_u8(remainder, null_terminator);

  if (out_mqtt_topic_length)
  {
    *out_mqtt_topic_length = (size_t)required_length;
  }

  return TI_OK;
}
