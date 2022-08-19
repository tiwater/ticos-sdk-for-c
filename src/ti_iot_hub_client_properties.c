// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_iot_hub_client_properties.h>

#include <ti_precondition_internal.h>
#include <ti_result_internal.h>

static const ti_span iot_hub_properties_reported = TI_SPAN_LITERAL_FROM_STR("reported");
static const ti_span iot_hub_properties_desired = TI_SPAN_LITERAL_FROM_STR("desired");
static const ti_span iot_hub_properties_desired_version = TI_SPAN_LITERAL_FROM_STR("$version");
static const ti_span properties_response_value_name = TI_SPAN_LITERAL_FROM_STR("value");
static const ti_span properties_ack_code_name = TI_SPAN_LITERAL_FROM_STR("ac");
static const ti_span properties_ack_version_name = TI_SPAN_LITERAL_FROM_STR("av");
static const ti_span properties_ack_description_name = TI_SPAN_LITERAL_FROM_STR("ad");

static const ti_span component_properties_label_name = TI_SPAN_LITERAL_FROM_STR("__t");
static const ti_span component_properties_label_value = TI_SPAN_LITERAL_FROM_STR("c");

TI_NODISCARD ti_result ti_iot_hub_client_properties_get_reported_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return ti_iot_hub_client_twin_patch_get_publish_topic(
      client, request_id, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_document_get_publish_topic(
    ti_iot_hub_client const* client,
    ti_span request_id,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return ti_iot_hub_client_twin_document_get_publish_topic(
      client, request_id, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_parse_received_topic(
    ti_iot_hub_client const* client,
    ti_span received_topic,
    ti_iot_hub_client_properties_message* out_message)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_message);

  ti_iot_hub_client_twin_response hub_twin_response;
  _ti_RETURN_IF_FAILED(
      ti_iot_hub_client_twin_parse_received_topic(client, received_topic, &hub_twin_response));

  out_message->request_id = hub_twin_response.request_id;
  out_message->message_type
      = (ti_iot_hub_client_properties_message_type)hub_twin_response.response_type;
  out_message->status = hub_twin_response.status;

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_begin_component(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer,
    ti_span component_name)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;

  _ti_RETURN_IF_FAILED(ti_json_writer_append_property_name(ref_json_writer, component_name));
  _ti_RETURN_IF_FAILED(ti_json_writer_append_begin_object(ref_json_writer));
  _ti_RETURN_IF_FAILED(
      ti_json_writer_append_property_name(ref_json_writer, component_properties_label_name));
  _ti_RETURN_IF_FAILED(
      ti_json_writer_append_string(ref_json_writer, component_properties_label_value));

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_end_component(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  return ti_json_writer_append_end_object(ref_json_writer);
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_begin_response_status(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer,
    ti_span property_name,
    int32_t status_code,
    int32_t version,
    ti_span description)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION_VALID_SPAN(property_name, 1, false);

  (void)client;

  _ti_RETURN_IF_FAILED(ti_json_writer_append_property_name(ref_json_writer, property_name));
  _ti_RETURN_IF_FAILED(ti_json_writer_append_begin_object(ref_json_writer));
  _ti_RETURN_IF_FAILED(
      ti_json_writer_append_property_name(ref_json_writer, properties_ack_code_name));
  _ti_RETURN_IF_FAILED(ti_json_writer_append_int32(ref_json_writer, status_code));
  _ti_RETURN_IF_FAILED(
      ti_json_writer_append_property_name(ref_json_writer, properties_ack_version_name));
  _ti_RETURN_IF_FAILED(ti_json_writer_append_int32(ref_json_writer, version));

  if (ti_span_size(description) != 0)
  {
    _ti_RETURN_IF_FAILED(
        ti_json_writer_append_property_name(ref_json_writer, properties_ack_description_name));
    _ti_RETURN_IF_FAILED(ti_json_writer_append_string(ref_json_writer, description));
  }

  _ti_RETURN_IF_FAILED(
      ti_json_writer_append_property_name(ref_json_writer, properties_response_value_name));

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_end_response_status(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  return ti_json_writer_append_end_object(ref_json_writer);
}

// Move reader to the value of property name
static ti_result json_child_token_move(ti_json_reader* ref_jr, ti_span property_name)
{
  do
  {
    if ((ref_jr->token.kind == TI_JSON_TOKEN_PROPERTY_NAME)
        && ti_json_token_is_text_equal(&(ref_jr->token), property_name))
    {
      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_jr));

      return TI_OK;
    }
    else if (ref_jr->token.kind == TI_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (ti_result_failed(ti_json_reader_skip_children(ref_jr)))
      {
        return TI_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (ref_jr->token.kind == TI_JSON_TOKEN_END_OBJECT)
    {
      return TI_ERROR_ITEM_NOT_FOUND;
    }
  } while (ti_result_succeeded(ti_json_reader_next_token(ref_jr)));

  return TI_ERROR_ITEM_NOT_FOUND;
}

// Check if the component name is in the model.  While this is sometimes
// indicated in the twin metadata (via "__t":"c" as a child), this metadata will NOT
// be specified during a TWIN PATCH operation.  Hence we cannot rely on it
// being present.  We instead use the application provided component_name list.
static bool is_component_in_model(
    ti_iot_hub_client const* client,
    ti_json_token const* component_name,
    ti_span* out_component_name)
{
  int32_t index = 0;

  while (index < client->_internal.options.component_names_length)
  {
    if (ti_json_token_is_text_equal(
            component_name, client->_internal.options.component_names[index]))
    {
      *out_component_name = client->_internal.options.component_names[index];
      return true;
    }

    index++;
  }

  return false;
}

TI_NODISCARD ti_result ti_iot_hub_client_properties_get_properties_version(
    ti_iot_hub_client const* client,
    ti_json_reader* ref_json_reader,
    ti_iot_hub_client_properties_message_type message_type,
    int32_t* out_version)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_reader);
  _ti_PRECONDITION(
      (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED)
      || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE));
  _ti_PRECONDITION_NOT_NULL(out_version);

  (void)client;

  _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));

  if (ref_json_reader->token.kind != TI_JSON_TOKEN_BEGIN_OBJECT)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));

  if (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)
  {
    _ti_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_properties_desired));
    _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));
  }

  _ti_RETURN_IF_FAILED(json_child_token_move(ref_json_reader, iot_hub_properties_desired_version));
  _ti_RETURN_IF_FAILED(ti_json_token_get_int32(&ref_json_reader->token, out_version));

  return TI_OK;
}

// process_first_move_if_needed performs initial setup when beginning to parse
// the JSON document.  It sets the next read token to the appropriate
// location based on whether we have a full twin or a patch and what property_type
// the application requested.
// Returns TI_OK if this is NOT the first read of the document.
static ti_result process_first_move_if_needed(
    ti_json_reader* jr,
    ti_iot_hub_client_properties_message_type message_type,
    ti_iot_hub_client_property_type property_type)

{
  if (jr->current_depth == 0)
  {
    _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));

    if (jr->token.kind != TI_JSON_TOKEN_BEGIN_OBJECT)
    {
      return TI_ERROR_UNEXPECTED_CHAR;
    }

    _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));

    if (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)
    {
      const ti_span property_to_query
          = (property_type == TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
          ? iot_hub_properties_reported
          : iot_hub_properties_desired;
      _ti_RETURN_IF_FAILED(json_child_token_move(jr, property_to_query));
      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));
    }
    return TI_OK;
  }
  else
  {
    // Not the first move so continue
    return TI_OK;
  }
}

// The underlying twin has various metadata embedded in the JSON.
// This metadata should not be passed back to the caller
// but should instead be silently ignored/skipped.
static ti_result skip_metadata_if_needed(
    ti_json_reader* jr,
    ti_iot_hub_client_properties_message_type message_type)
{
  while (true)
  {
    // Within the "root" or "component name" section
    if ((message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
         && jr->current_depth == 1)
        || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
            && jr->current_depth == 2))
    {
      if ((ti_json_token_is_text_equal(&jr->token, iot_hub_properties_desired_version)))
      {
        // Skip version property name and property value
        _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));
        _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return TI_OK;
      }
    }
    // Within the property value section
    else if (
        (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
         && jr->current_depth == 2)
        || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
            && jr->current_depth == 3))
    {
      if (ti_json_token_is_text_equal(&jr->token, component_properties_label_name))
      {
        // Skip label property name and property value
        _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));
        _ti_RETURN_IF_FAILED(ti_json_reader_next_token(jr));

        continue;
      }
      else
      {
        return TI_OK;
      }
    }
    else
    {
      return TI_OK;
    }
  }
}

// verify_valid_json_position makes sure that the ti_json_reader
// is in a good state.  Applications modify the ti_json_reader as they
// traverse properties and a poorly written application could leave
// it in an invalid state.
static ti_result verify_valid_json_position(
    ti_json_reader* jr,
    ti_iot_hub_client_properties_message_type message_type,
    ti_span component_name)
{
  // Not on a property name or end of object
  if (jr->current_depth != 0
      && (jr->token.kind != TI_JSON_TOKEN_PROPERTY_NAME
          && jr->token.kind != TI_JSON_TOKEN_END_OBJECT))
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  // Component property - In user property value object
  if ((message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
       && jr->current_depth > 2)
      || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
          && jr->current_depth > 3))
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  // Non-component property - In user property value object
  if ((ti_span_size(component_name) == 0)
      && ((message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
           && jr->current_depth > 1)
          || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
              && jr->current_depth > 2)))
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  return TI_OK;
}

/*
Assuming a JSON of either the below types

TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED:

{
  //ROOT COMPONENT or COMPONENT NAME section
  "component_one": {
    //PROPERTY VALUE section
    "prop_one": 1,
    "prop_two": "string"
  },
  "component_two": {
    "prop_three": 45,
    "prop_four": "string"
  },
  "not_component": 42,
  "$version": 5
}

TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE:

{
  "desired": {
    //ROOT COMPONENT or COMPONENT NAME section
    "component_one": {
        //PROPERTY VALUE section
        "prop_one": 1,
        "prop_two": "string"
    },
    "component_two": {
        "prop_three": 45,
        "prop_four": "string"
    },
    "not_component": 42,
    "$version": 5
  },
  "reported": {
      "manufacturer": "Sample-Manufacturer",
      "model": "pnp-sample-Model-123",
      "swVersion": "1.0.0.0",
      "osName": "Contoso"
  }
}

*/
TI_NODISCARD ti_result ti_iot_hub_client_properties_get_next_component_property(
    ti_iot_hub_client const* client,
    ti_json_reader* ref_json_reader,
    ti_iot_hub_client_properties_message_type message_type,
    ti_iot_hub_client_property_type property_type,
    ti_span* out_component_name)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_NOT_NULL(ref_json_reader);
  _ti_PRECONDITION_NOT_NULL(out_component_name);
  _ti_PRECONDITION(
      (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED)
      || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE));
  _ti_PRECONDITION(
      (property_type == TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
      || (property_type == TI_IOT_HUB_CLIENT_PROPERTY_WRITABLE));
  _ti_PRECONDITION(
      (property_type == TI_IOT_HUB_CLIENT_PROPERTY_WRITABLE)
      || ((property_type == TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE)
          && (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE)));

  _ti_RETURN_IF_FAILED(
      verify_valid_json_position(ref_json_reader, message_type, *out_component_name));
  _ti_RETURN_IF_FAILED(process_first_move_if_needed(ref_json_reader, message_type, property_type));

  while (true)
  {
    _ti_RETURN_IF_FAILED(skip_metadata_if_needed(ref_json_reader, message_type));

    if (ref_json_reader->token.kind == TI_JSON_TOKEN_END_OBJECT)
    {
      // We've read all the children of the current object we're traversing.
      if ((message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
           && ref_json_reader->current_depth == 0)
          || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
              && ref_json_reader->current_depth == 1))
      {
        // We've read the last child the root of the JSON tree we're traversing.  We're done.
        return TI_ERROR_IOT_END_OF_PROPERTIES;
      }

      // There are additional tokens to read.  Continue.
      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));
      continue;
    }

    break;
  }

  if ((message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED
       && ref_json_reader->current_depth == 1)
      || (message_type == TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE
          && ref_json_reader->current_depth == 2))
  {
    // Retrieve the next property/component pair.
    if (is_component_in_model(client, &ref_json_reader->token, out_component_name))
    {
      // Properties that are children of components are simply modeled as JSON children
      // in the underlying twin.  Traverse into the object.
      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));

      if (ref_json_reader->token.kind != TI_JSON_TOKEN_BEGIN_OBJECT)
      {
        return TI_ERROR_UNEXPECTED_CHAR;
      }

      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));
      _ti_RETURN_IF_FAILED(skip_metadata_if_needed(ref_json_reader, message_type));
    }
    else
    {
      // The current property is not the child of a component.
      *out_component_name = TI_SPAN_EMPTY;
    }
  }

  return TI_OK;
}
