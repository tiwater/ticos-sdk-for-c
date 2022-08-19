// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief 定义 #ti_result 以及相关函数
 *
 */

#ifndef _ti_RESULT_H
#define _ti_RESULT_H

#include <stdbool.h>
#include <stdint.h>

#include <_ti_cfg_prefix.h>

enum
{
  _ti_FACILITY_CORE = 0x1,
  _ti_FACILITY_CORE_PLATFORM = 0x2,
  _ti_FACILITY_CORE_JSON = 0x3,
  _ti_FACILITY_CORE_HTTP = 0x4,
  _ti_FACILITY_IOT = 0x5,
  _ti_FACILITY_IOT_MQTT = 0x6,
  _ti_FACILITY_ULIB = 0x7,
};

enum
{
  _ti_ERROR_FLAG = (int32_t)0x80000000,
};

/**
 * @brief 成功或者错误的结果定义
 *
 * @note 参见其它头文件中的 `ti_result` 值:
 * - #ti_result_core
 * - #ti_result_iot
 */
typedef int32_t ti_result;

// ti_result Bits:
//   - 31 Severity (0 - success, 1 - failure).
//   - 16..30 Facility.
//   - 0..15 Code.

#define _ti_RESULT_MAKE_ERROR(facility, code) \
  ((ti_result)((uint32_t)_ti_ERROR_FLAG | ((uint32_t)(facility) << 16U) | (uint32_t)(code)))

#define _ti_RESULT_MAKE_SUCCESS(facility, code) \
  ((ti_result)(((uint32_t)(facility) << 16U) | (uint32_t)(code)))

/**
 * @brief SDK Core 中的 #ti_result 成功或者错误结果定义.
 */
enum ti_result_core
{
  // === Core: Success results ====
  /// Success.
  TI_OK = _ti_RESULT_MAKE_SUCCESS(_ti_FACILITY_CORE, 0),

  // === Core: Error results ===
  /// A context was canceled, and a function had to return before result was ready.
  TI_ERROR_CANCELED = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 0),

  /// Input argument does not comply with the expected range of values.
  TI_ERROR_ARG = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 1),

  /// The destination size is too small for the operation.
  TI_ERROR_NOT_ENOUGH_SPACE = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 2),

  /// Requested functionality is not implemented.
  TI_ERROR_NOT_IMPLEMENTED = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 3),

  /// Requested item was not found.
  TI_ERROR_ITEM_NOT_FOUND = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 4),

  /// Input can't be successfully parsed.
  TI_ERROR_UNEXPECTED_CHAR = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 5),

  /// Unexpected end of the input data.
  TI_ERROR_UNEXPECTED_END = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 6),

  /// Not supported.
  TI_ERROR_NOT_SUPPORTED = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 7),

  /// An external dependency required to perform the operation was not provided. The operation needs
  /// an implementation of the platform layer or an HTTP transport adapter.
  TI_ERROR_DEPENDENCY_NOT_PROVIDED = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE, 8),

  // === Platform ===
  /// Dynamic memory allocation request was not successful.
  TI_ERROR_OUT_OF_MEMORY = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_PLATFORM, 1),

  // === JSON error codes ===
  /// The kind of the token being read is not compatible with the expected type of the value.
  TI_ERROR_JSON_INVALID_STATE = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_JSON, 1),

  /// The JSON depth is too large.
  TI_ERROR_JSON_NESTING_OVERFLOW = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_JSON, 2),

  /// No more JSON text left to process.
  TI_ERROR_JSON_READER_DONE = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_JSON, 3),

  // === HTTP error codes ===
  /// The #ti_http_response instance is in an invalid state.
  TI_ERROR_HTTP_INVALID_STATE = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 1),

  /// HTTP pipeline is malformed.
  TI_ERROR_HTTP_PIPELINE_INVALID_POLICY = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 2),

  /// Unknown HTTP method verb.
  TI_ERROR_HTTP_INVALID_METHOD_VERB = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 3),

  /// Authentication failed.
  TI_ERROR_HTTP_AUTHENTICATION_FAILED = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 4),

  /// HTTP response overflow.
  TI_ERROR_HTTP_RESPONSE_OVERFLOW = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 5),

  /// Couldn't resolve host.
  TI_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 6),

  /// Error while parsing HTTP response header.
  TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 7),

  /// There are no more headers within the HTTP response payload.
  TI_ERROR_HTTP_END_OF_HEADERS = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 8),

  // === HTTP Adapter error codes ===
  /// Generic error in the HTTP transport adapter implementation.
  TI_ERROR_HTTP_ADAPTER = _ti_RESULT_MAKE_ERROR(_ti_FACILITY_CORE_HTTP, 9),
};

/**
 * @brief Checks whether the \p result provided indicates a failure.
 *
 * @param[in] result Result value to check for failure.
 *
 * @return `true` if the operation that returned this \p result failed, otherwise return `false`.
 */
TI_NODISCARD TI_INLINE bool ti_result_failed(ti_result result)
{
  return ((uint32_t)result & (uint32_t)_ti_ERROR_FLAG) != 0;
}

/**
 * @brief Checks whether the \p result provided indicates a success.
 *
 * @param[in] result Result value to check for success.
 *
 * @return `true` if the operation that returned this \p result was successful, otherwise return
 * `false`.
 */
TI_NODISCARD TI_INLINE bool ti_result_succeeded(ti_result result)
{
  return !ti_result_failed(result);
}

#include <_ti_cfg_suffix.h>

#endif // _ti_RESULT_H
