// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines internals used by http.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_HTTP_INTERNAL_H
#define _ti_HTTP_INTERNAL_H

#include <ti_context.h>
#include <ti_http.h>
#include <ti_http_transport.h>
#include <ti_result.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg_prefix.h>

enum
{
  /// The maximum number of HTTP pipeline policies allowed.
  _ti_MAXIMUM_NUMBER_OF_POLICIES = 10,
};

/**
 * @brief Internal definition of an HTTP pipeline.
 * Defines the number of policies inside a pipeline.
 */
typedef struct
{
  struct
  {
    _ti_http_policy policies[_ti_MAXIMUM_NUMBER_OF_POLICIES];
  } _internal;
} _ti_http_pipeline;

typedef enum
{
  _ti_http_policy_apiversion_option_location_header,
  _ti_http_policy_apiversion_option_location_queryparameter
} _ti_http_policy_apiversion_option_location;

/**
 * @brief Defines the options structure used by the API Version policy.
 */
typedef struct
{
  // Services pass API versions in the header or in query parameters
  struct
  {
    ti_span name;
    ti_span version;

    // Avoid using enum as the first field within structs, to allow for { 0 } initialization.
    // This is a workaround for IAR compiler warning [Pe188]: enumerated type mixed with another
    // type.

    _ti_http_policy_apiversion_option_location option_location;
  } _internal;
} _ti_http_policy_apiversion_options;

/**
 * @brief options for the telemetry policy
 * os = string representation of currently executing Operating System
 *
 */
typedef struct
{
  ti_span component_name;
} _ti_http_policy_telemetry_options;

/**
 * @brief Creates _ti_http_policy_telemetry_options with default values.
 *
 * @param[in] component_name The name of the SDK component.
 *
 * @return Initialized telemetry options.
 */
TI_NODISCARD TI_INLINE _ti_http_policy_telemetry_options
_ti_http_policy_telemetry_options_create(ti_span component_name)
{
  _ti_PRECONDITION_VALID_SPAN(component_name, 1, false);
  return (_ti_http_policy_telemetry_options){ .component_name = component_name };
}

TI_NODISCARD TI_INLINE _ti_http_policy_apiversion_options
_ti_http_policy_apiversion_options_default()
{
  return (_ti_http_policy_apiversion_options){
    ._internal = { .name = TI_SPAN_EMPTY,
                   .version = TI_SPAN_EMPTY,
                   .option_location = _ti_http_policy_apiversion_option_location_header }
  };
}

/**
 * @brief Initialize ti_http_policy_retry_options with default values
 *
 */
TI_NODISCARD ti_http_policy_retry_options _ti_http_policy_retry_options_default();

// PipelinePolicies
//   Policies are non-allocating caveat the TransportPolicy
//   Transport policies can only allocate if the transport layer they call allocates
// Client ->
//  ===HttpPipelinePolicies===
//    UniqueRequestID
//    Retry
//    Authentication
//    Logging
//    Buffer Response
//    Distributed Tracing
//    TransportPolicy
//  ===Transport Layer===
// PipelinePolicies must implement the process function
//

// Start the pipeline
TI_NODISCARD ti_result ti_http_pipeline_process(
    _ti_http_pipeline* ref_pipeline,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

TI_NODISCARD ti_result ti_http_pipeline_policy_apiversion(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

TI_NODISCARD ti_result ti_http_pipeline_policy_telemetry(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

TI_NODISCARD ti_result ti_http_pipeline_policy_retry(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

TI_NODISCARD ti_result ti_http_pipeline_policy_credential(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

#ifndef TI_NO_LOGGING
TI_NODISCARD ti_result ti_http_pipeline_policy_logging(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);
#endif // TI_NO_LOGGING

TI_NODISCARD ti_result ti_http_pipeline_policy_transport(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

TI_NODISCARD TI_INLINE ti_result _ti_http_pipeline_nextpolicy(
    _ti_http_policy* ref_policies,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  // Transport Policy is the last policy in the pipeline
  //  it returns without calling nextpolicy
  if (ref_policies[0]._internal.process == NULL)
  {
    return TI_ERROR_HTTP_PIPELINE_INVALID_POLICY;
  }

  return ref_policies[0]._internal.process(
      &(ref_policies[1]), ref_policies[0]._internal.options, ref_request, ref_response);
}

/**
 * @brief Format buffer as a http request containing URL and header spans.
 *
 * @remark The initial \p url provided by the caller is expected to already be url-encoded.
 *
 * @param[out] out_request HTTP request to initialize.
 * @param[in] context A pointer to an #ti_context node.
 * @param[in] method HTTP verb: `"GET"`, `"POST"`, etc.
 * @param[in] url The #ti_span to be used for storing the url. An initial value is expected to be in
 * the buffer containing url schema and the server address. It can contain query parameters (like
 * https://service.ticos.cc?query=1). This value is expected to be url-encoded.
 * @param[in] url_length The size of the initial url value within url #ti_span.
 * @param[in] headers_buffer The #ti_span to be used for storing headers for the request. The total
 * number of headers are calculated automatically based on the size of the buffer.
 * @param[in] body The #ti_span buffer that contains a payload for the request. Use #TI_SPAN_EMPTY
 * for requests that don't have a body.
 *
 * @return
 *   - *`TI_OK`* success.
 *   - *`TI_ERROR_ARG`*
 *     - `out_request` is _NULL_.
 *     - `url`, `method`, or `headers_buffer` are invalid spans (see @ref _ti_span_is_valid).
 */
TI_NODISCARD ti_result ti_http_request_init(
    ti_http_request* out_request,
    ti_context* context,
    ti_http_method method,
    ti_span url,
    int32_t url_length,
    ti_span headers_buffer,
    ti_span body);

/**
 * @brief Set a query parameter at the end of url.
 *
 * @remark Query parameters are stored url-encoded. This function will not check if
 * the a query parameter already exists in the URL. Calling this function twice with same \p name
 * would duplicate the query parameter.
 *
 * @param[out] ref_request HTTP request that holds the URL to set the query parameter to.
 * @param[in] name URL parameter name.
 * @param[in] value URL parameter value.
 * @param[in] \p is_value_url_encoded boolean value that defines if the query parameter (name and
 * value) is url-encoded or not.
 *
 * @remarks if \p is_value_url_encoded is set to false, before setting query parameter, it would be
 * url-encoded.
 *
 * @return
 *   - *`TI_OK`* success.
 *   - *`TI_ERROR_NOT_ENOUGH_SPACE`* the `URL` would grow past the `max_url_size`, should
 * the parameter get set.
 *   - *`TI_ERROR_ARG`*
 *     - `p_request` is _NULL_.
 *     - `name` or `value` are invalid spans (see @ref _ti_span_is_valid).
 *     - `name` or `value` are empty.
 *     - `name`'s or `value`'s buffer overlap resulting `url`'s buffer.
 */
TI_NODISCARD ti_result ti_http_request_set_query_parameter(
    ti_http_request* ref_request,
    ti_span name,
    ti_span value,
    bool is_value_url_encoded);

/**
 * @brief Add a new HTTP header for the request.
 *
 * @param ref_request HTTP request builder that holds the URL to set the query parameter to.
 * @param name Header name (e.g. `"Content-Type"`).
 * @param value Header value (e.g. `"application/x-www-form-urlencoded"`).
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_NOT_ENOUGH_SPACE There isn't enough space in the \p ref_request to add a
 * header.
 */
TI_NODISCARD ti_result
ti_http_request_append_header(ti_http_request* ref_request, ti_span name, ti_span value);

#include <_ti_cfg_suffix.h>

#endif // _ti_HTTP_INTERNAL_H
