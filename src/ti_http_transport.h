// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Utilities to be used by HTTP transport policy implementations.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_HTTP_TRANSPORT_H
#define _ti_HTTP_TRANSPORT_H

#include <ti_http.h>
#include <ti_span.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief A type representing an HTTP method (`POST`, `PUT`, `GET`, `DELETE`, etc.).
 */
typedef ti_span ti_http_method;

/**
 * @brief HTTP GET method name.
 */
TI_INLINE ti_http_method ti_http_method_get() { return TI_SPAN_FROM_STR("GET"); }

/**
 * @brief HTTP HEAD method name.
 */
TI_INLINE ti_http_method ti_http_method_head() { return TI_SPAN_FROM_STR("HEAD"); }

/**
 * @brief HTTP POST method name.
 */
TI_INLINE ti_http_method ti_http_method_post() { return TI_SPAN_FROM_STR("POST"); }

/**
 * @brief HTTP PUT method name.
 */
TI_INLINE ti_http_method ti_http_method_put() { return TI_SPAN_FROM_STR("PUT"); }

/**
 * @brief HTTP DELETE method name.
 */
TI_INLINE ti_http_method ti_http_method_delete() { return TI_SPAN_FROM_STR("DELETE"); }

/**
 * @brief HTTP PATCH method name.
 */
TI_INLINE ti_http_method ti_http_method_patch() { return TI_SPAN_FROM_STR("PATCH"); }

/**
 * @brief Represents a name/value pair of #ti_span instances.
 */
typedef struct
{
  ti_span name; ///< Name.
  ti_span value; ///< Value.
} _ti_http_request_header;

/**
 * @brief A type representing a buffer of #_ti_http_request_header instances for HTTP request
 * headers.
 */
typedef ti_span _ti_http_request_headers;

/**
 * @brief Structure used to represent an HTTP request.
 * It contains an HTTP method, URL, headers and body. It also contains
 * another utility variables.
 */
typedef struct
{
  struct
  {
    ti_context* context;
    ti_http_method method;
    ti_span url;
    int32_t url_length;
    int32_t query_start;
    _ti_http_request_headers headers; // Contains instances of _ti_http_request_header
    int32_t headers_length;
    int32_t max_headers;
    int32_t retry_headers_start_byte_offset;
    ti_span body;
  } _internal;
} ti_http_request;

/**
 * @brief Used to declare policy process callback #_ti_http_policy_process_fn definition.
 */
// Definition is below.
typedef struct _ti_http_policy _ti_http_policy;

/**
 * @brief Defines the callback signature of a policy process which should receive an
 * #_ti_http_policy, options reference (as `void*`), an #ti_http_request and an #ti_http_response.
 *
 * @remark `void*` is used as polymorphic solution for any policy. Each policy implementation would
 * know the specific pointer type to cast options to.
 */
typedef TI_NODISCARD ti_result (*_ti_http_policy_process_fn)(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response);

/**
 * @brief HTTP policy.
 * An HTTP pipeline inside SDK clients is an array of HTTP policies.
 */
struct _ti_http_policy
{
  struct
  {
    _ti_http_policy_process_fn process;
    void* options;
  } _internal;
};

/**
 * @brief Gets the HTTP header by index.
 *
 * @param[in] request HTTP request to get HTTP header from.
 * @param[in] index Index of the HTTP header to get.
 * @param[out] out_name A pointer to an #ti_span to write the header's name.
 * @param[out] out_value A pointer to an #ti_span to write the header's value.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_ARG \p index is out of range.
 */
TI_NODISCARD ti_result ti_http_request_get_header(
    ti_http_request const* request,
    int32_t index,
    ti_span* out_name,
    ti_span* out_value);

/**
 * @brief Get method of an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the method.
 * @param[out] out_method Pointer to write the HTTP method to.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval other Failure.
 */
TI_NODISCARD ti_result
ti_http_request_get_method(ti_http_request const* request, ti_http_method* out_method);

/**
 * @brief Get the URL from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the URL.
 * @param[out] out_url Pointer to write the HTTP URL to.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval other Failure.
 */
TI_NODISCARD ti_result ti_http_request_get_url(ti_http_request const* request, ti_span* out_url);

/**
 * @brief Get body from an HTTP request.
 *
 * @remarks This function is expected to be used by transport layer only.
 *
 * @param[in] request The HTTP request from which to get the body.
 * @param[out] out_body Pointer to write the HTTP request body to.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval other Failure.
 */
TI_NODISCARD ti_result ti_http_request_get_body(ti_http_request const* request, ti_span* out_body);

/**
 * @brief This function is expected to be used by transport adapters like curl. Use it to write
 * content from \p source to \p ref_response.
 *
 * @remarks The \p source can be an empty #ti_span. If so, nothing will be written.
 *
 * @param[in,out] ref_response Pointer to an #ti_http_response.
 * @param[in] source This is an #ti_span with the content to be written into \p ref_response.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_NOT_ENOUGH_SPACE The \p response buffer is not big enough to contain the \p
 * source content.
 */
TI_NODISCARD ti_result ti_http_response_append(ti_http_response* ref_response, ti_span source);

/**
 * @brief Returns the number of headers within the request.
 *
 * @param[in] request Pointer to an #ti_http_request to be used by this function.
 *
 * @return Number of headers in the \p request.
 */
TI_NODISCARD int32_t ti_http_request_headers_count(ti_http_request const* request);

/**
 * @brief Sends an HTTP request through the wire and write the response into \p ref_response.
 *
 * @param[in] request Points to an #ti_http_request that contains the settings and data that is
 * used to send the request through the wire.
 * @param[in,out] ref_response Points to an #ti_http_response where the response from the wire will
 * be written.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_HTTP_RESPONSE_OVERFLOW There was an issue while trying to write into \p
 * ref_response. It might mean that there was not enough space in \p ref_response to hold the entire
 * response from the network.
 * @retval #TI_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST The URL from \p ref_request can't be
 * resolved by the HTTP stack and the request was not sent.
 * @retval #TI_ERROR_HTTP_ADAPTER Any other issue from the transport adapter layer.
 * @retval #TI_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 */
TI_NODISCARD ti_result
ti_http_client_send_request(ti_http_request const* request, ti_http_response* ref_response);

#include <_ti_cfg_suffix.h>

#endif // _ti_HTTP_TRANSPORT_H
