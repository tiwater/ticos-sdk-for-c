// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #ti_result related internal helper functions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_RESULT_INTERNAL_H
#define _ti_RESULT_INTERNAL_H

#include <ti_result.h>
#include <ti_span.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Convenience macro to return if an operation failed.
 */
#define _ti_RETURN_IF_FAILED(exp)       \
  do                                    \
  {                                     \
    ti_result const _ti_result = (exp); \
    if (ti_result_failed(_ti_result))   \
    {                                   \
      return _ti_result;                \
    }                                   \
  } while (0)

/**
 * @brief Convenience macro to return if the provided span is not of the expected, required size.
 */
#define _ti_RETURN_IF_NOT_ENOUGH_SIZE(span, required_size) \
  do                                                       \
  {                                                        \
    int32_t const _ti_req_sz = (required_size);            \
    if (ti_span_size(span) < _ti_req_sz || _ti_req_sz < 0) \
    {                                                      \
      return TI_ERROR_NOT_ENOUGH_SPACE;                    \
    }                                                      \
  } while (0)

#include <_ti_cfg_suffix.h>

#endif // _ti_RESULT_INTERNAL_H
