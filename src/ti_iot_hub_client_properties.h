// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition for the IoT Plug and Play properties writer and parsing routines.
 */

#ifndef _ti_IOT_HUB_CLIENT_PROPERTIES_H
#define _ti_IOT_HUB_CLIENT_PROPERTIES_H

#include <stdbool.h>
#include <stdint.h>

#include <ti_json.h>
#include <ti_result.h>
#include <ti_span.h>

#include <ti_iot_hub_client.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Append the necessary characters to a reported properties JSON payload belonging to a
 * component.
 *
 * The payload will be of the form:
 *
 * @code
 * "<component_name>": {
 *     "__t": "c",
 *     "temperature": 23
 * }
 * @endcode
 *
 * @note This API only writes the metadata for a component's properties.  The
 * application itself must specify the payload contents between calls
 * to this API and ti_iot_hub_client_properties_writer_end_component() using
 * \p ref_json_writer to specify the JSON payload.
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_writer The #ti_json_writer to append the necessary characters for an IoT
 * Plug and Play component.
 * @param[in] component_name The component name associated with the reported properties.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 * @pre \p component_name must be a valid, non-empty #ti_span.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK The JSON payload was prefixed successfully.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_begin_component(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer,
    ti_span component_name);

/**
 * @brief Append the necessary characters to end a reported properties JSON payload belonging to a
 * component.
 *
 * @note This API should be used in conjunction with
 * ti_iot_hub_client_properties_writer_begin_component().
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_writer The #ti_json_writer to append the necessary characters for an IoT
 * Plug and Play component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK The JSON payload was suffixed successfully.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_end_component(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer);

/**
 * @brief Begin a property response to a writable property request from the service.
 *
 * This API should be used in response to an incoming writable properties. More details can be
 * found here:
 *
 * https://docs.microsoft.com/ticos/iot-pnp/concepts-convention#writable-properties
 *
 * The payload will be of the form:
 *
 * **Without component**
 * @code
 * {
 *   "<property_name>":{
 *     "ac": <status_code>,
 *     "av": <version>,
 *     "ad": "<description>",
 *     "value": <user_value>
 *   }
 * }
 * @endcode
 *
 * **With component**
 * @code
 * {
 *   "<component_name>": {
 *     "__t": "c",
 *     "<property_name>": {
 *       "ac": <status_code>,
 *       "av": <version>,
 *       "ad": "<description>",
 *       "value": <user_value>
 *     }
 *   }
 * }
 * @endcode
 *
 * To send a status for properties belonging to a component, first call the
 * ti_iot_hub_client_properties_writer_begin_component() API to prefix the payload with the
 * necessary identification. The API call flow would look like the following with the listed JSON
 * payload being generated.
 *
 * @code
 * ti_iot_hub_client_properties_writer_begin_component()
 * ti_iot_hub_client_properties_writer_begin_response_status()
 * // Append user value here (<user_value>) using ref_json_writer directly.
 * ti_iot_hub_client_properties_writer_end_response_status()
 * ti_iot_hub_client_properties_writer_end_component()
 * @endcode
 *
 * @note This API only writes the metadata for the properties response.  The
 * application itself must specify the payload contents between calls
 * to this API and ti_iot_hub_client_properties_writer_end_response_status() using
 * \p ref_json_writer to specify the JSON payload.
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_writer The initialized #ti_json_writer to append data to.
 * @param[in] property_name The name of the property to write a response payload for.
 * @param[in] status_code The HTTP-like status code to respond with. See #ti_iot_status for
 * possible supported values.
 * @param[in] version The version of the property the application is acknowledging.
 * This can be retrieved from the service request by
 * calling ti_iot_hub_client_properties_get_properties_version.
 * @param[in] description An optional description detailing the context or any details about
 * the acknowledgement. This can be #TI_SPAN_EMPTY.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 * @pre \p property_name must be a valid, non-empty #ti_span.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK The JSON payload was prefixed successfully.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_begin_response_status(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer,
    ti_span property_name,
    int32_t status_code,
    int32_t version,
    ti_span description);

/**
 * @brief End a properties response payload with confirmation status.
 *
 * @note This API should be used in conjunction with
 * ti_iot_hub_client_properties_writer_begin_response_status().
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_writer The initialized #ti_json_writer to append data to.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_writer must not be `NULL`.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK The JSON payload was suffixed successfully.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_writer_end_response_status(
    ti_iot_hub_client const* client,
    ti_json_writer* ref_json_writer);

/**
 * @brief Read the IoT Plug and Play property version.
 *
 * @warning This modifies the state of the json reader. To then use the same json reader
 * with ti_iot_hub_client_properties_get_next_component_property(), you must call
 * ti_json_reader_init() again after this call and before the call to
 * ti_iot_hub_client_properties_get_next_component_property() or make an additional copy before
 * calling this API.
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_reader The pointer to the #ti_json_reader used to parse through the JSON
 * payload.
 * @param[in] message_type The #ti_iot_hub_client_properties_message_type representing the message
 * type associated with the payload.
 * @param[out] out_version The numeric version of the properties in the JSON payload.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_reader must not be `NULL`.
 * @pre \p message_type must be `TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED` or
 * `TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE`.
 * @pre \p out_version must not be `NULL`.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK If the function returned a valid version.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_get_properties_version(
    ti_iot_hub_client const* client,
    ti_json_reader* ref_json_reader,
    ti_iot_hub_client_properties_message_type message_type,
    int32_t* out_version);

/**
 * @brief Property type
 *
 */
typedef enum
{
  /** @brief Property was originally reported from the device. */
  TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE,
  /** @brief Property was received from the service. */
  TI_IOT_HUB_CLIENT_PROPERTY_WRITABLE
} ti_iot_hub_client_property_type;

/**
 * @brief Iteratively read the IoT Plug and Play component properties.
 *
 * Note that between calls, the #ti_span pointed to by \p out_component_name shall not be modified,
 * only checked and compared. Internally, the #ti_span is only changed if the component name changes
 * in the JSON document and is not necessarily set every invocation of the function.
 *
 * On success, the `ref_json_reader` will be set on a valid property name. After checking the
 * property name, the reader can be advanced to the property value by calling
 * ti_json_reader_next_token(). Note that on the subsequent call to this API, it is expected that
 * the json reader will be placed AFTER the read property name and value. That means that after
 * reading the property value (including single values or complex objects), the user must call
 * ti_json_reader_next_token().
 *
 * Below is a code snippet which you can use as a starting point:
 *
 * @code
 *
 * while (ti_result_succeeded(ti_iot_hub_client_properties_get_next_component_property(
 *       &hub_client, &jr, message_type, TI_IOT_HUB_CLIENT_PROPERTY_WRITABLE, &component_name)))
 * {
 *   // Check if property is of interest (substitute user_property for your own property name)
 *   if (ti_json_token_is_text_equal(&jr.token, user_property))
 *   {
 *     ti_json_reader_next_token(&jr);
 *
 *     // Get the property value here
 *     // Example: ti_json_token_get_int32(&jr.token, &user_int);
 *
 *     // Skip to next property value
 *     ti_json_reader_next_token(&jr);
 *   }
 *   else
 *   {
 *     // The JSON reader must be advanced regardless of whether the property
 *     // is of interest or not.
 *     ti_json_reader_next_token(&jr);
 *
 *     // Skip children in case the property value is an object
 *     ti_json_reader_skip_children(&jr);
 *     ti_json_reader_next_token(&jr);
 *   }
 * }
 *
 * @endcode
 *
 * @warning If you need to retrieve more than one \p property_type, you should first complete the
 * scan of all components for the first property type (until the API returns
 * #TI_ERROR_IOT_END_OF_PROPERTIES). Then you must call ti_json_reader_init() again after this call
 * and before the next call to ti_iot_hub_client_properties_get_next_component_property with the
 * different \p property_type.
 *
 * @param[in] client The #ti_iot_hub_client to use for this call.
 * @param[in,out] ref_json_reader The #ti_json_reader to parse through. The ownership of iterating
 * through this json reader is shared between the user and this API.
 * @param[in] message_type The #ti_iot_hub_client_properties_message_type representing the message
 * type associated with the payload.
 * @param[in] property_type The #ti_iot_hub_client_property_type to scan for.
 * @param[out] out_component_name The #ti_span* representing the value of the component.
 *
 * @pre \p client must not be `NULL`.
 * @pre \p ref_json_reader must not be `NULL`.
 * @pre \p out_component_name must not be `NULL`. It must point to an #ti_span instance.
 * @pre \p message_type must be `TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED` or
 * `TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE`.
 * @pre \p property_type must be `TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE` or
 * `TI_IOT_HUB_CLIENT_PROPERTY_WRITABLE`.
 * @pre \p If `TI_IOT_HUB_CLIENT_PROPERTY_REPORTED_FROM_DEVICE` is specified in \p property_type,
 * then \p message_type must be `TI_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_GET_RESPONSE`.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK If the function returned a valid #ti_json_reader pointing to the property name and
 * the #ti_span with a component name.
 * @retval #TI_ERROR_JSON_INVALID_STATE If the json reader is passed in at an unexpected location.
 * @retval #TI_ERROR_IOT_END_OF_PROPERTIES If there are no more properties left for the component.
 */
TI_NODISCARD ti_result ti_iot_hub_client_properties_get_next_component_property(
    ti_iot_hub_client const* client,
    ti_json_reader* ref_json_reader,
    ti_iot_hub_client_properties_message_type message_type,
    ti_iot_hub_client_property_type property_type,
    ti_span* out_component_name);

#include <_ti_cfg_suffix.h>

#endif //_ti_IOT_HUB_CLIENT_PROPERTIES_H
