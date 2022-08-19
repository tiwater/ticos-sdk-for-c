// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines private implementation used by http.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_HTTP_PRIVATE_H
#define _ti_HTTP_PRIVATE_H

#include <ti_http.h>
#include <ti_http_transport.h>
#include <ti_precondition.h>
#include <ti_span.h>
#include <ti_precondition_internal.h>

#include <stdbool.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Mark that the HTTP headers that are gong to be added via
 * `ti_http_request_append_header` are going to be considered as retry headers.
 *
 * @param ref_request HTTP request.
 *
 * @return
 *   - *`TI_OK`* success.
 *   - *`TI_ERROR_ARG`* `ref_request` is _NULL_.
 */
TI_NODISCARD TI_INLINE ti_result
_ti_http_request_mark_retry_headers_start(ti_http_request* ref_request)
{
  _ti_PRECONDITION_NOT_NULL(ref_request);
  ref_request->_internal.retry_headers_start_byte_offset
      = ref_request->_internal.headers_length * (int32_t)sizeof(_ti_http_request_header);
  return TI_OK;
}

TI_NODISCARD TI_INLINE ti_result _ti_http_request_remove_retry_headers(ti_http_request* ref_request)
{
  _ti_PRECONDITION_NOT_NULL(ref_request);
  ref_request->_internal.headers_length = ref_request->_internal.retry_headers_start_byte_offset
      / (int32_t)sizeof(_ti_http_request_header);
  return TI_OK;
}

/**
 * @brief Sets buffer and parser to its initial state.
 *
 */
void _ti_http_response_reset(ti_http_response* ref_response);

#include <_ti_cfg_suffix.h>

#endif // _ti_HTTP_PRIVATE_H
