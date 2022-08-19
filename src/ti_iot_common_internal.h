// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file ti_iot_common.h
 *
 * @brief Ticos IoT common definitions.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_IOT_CORE_INTERNAL_H
#define _ti_IOT_CORE_INTERNAL_H

#include <ti_result.h>
#include <ti_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Gives the length, in bytes, of the string that would represent the given number.
 *
 * @param[in] number The number whose length, as a string, is to be evaluated.
 * @return The length (not considering null terminator) of the string that would represent the given
 * number.
 */
TI_NODISCARD int32_t _ti_iot_u32toa_size(uint32_t number);

/**
 * @brief Gives the length, in bytes, of the string that would represent the given number.
 *
 * @param[in] number The number whose length, as a string, is to be evaluated.
 * @return The length (not considering null terminator) of the string that would represent the given
 * number.
 */
TI_NODISCARD int32_t _ti_iot_u64toa_size(uint64_t number);

/**
 * @brief Copies the url-encoded content of `source` span into `destination`, returning the free
 * remaining of `destination`.
 *
 * @param[in] destination The span where the `source` is url-encoded to.
 * @param[in] source The span to url-encode and copy the content from.
 * @param[out] out_remainder A slice of `destination` with the non-used buffer portion of
 * `destination`.
 * @return An `ti_result` value.
 */
TI_NODISCARD ti_result
_ti_span_copy_url_encode(ti_span destination, ti_span source, ti_span* out_remainder);

#include <_ti_cfg_suffix.h>

#endif // _ti_IOT_CORE_INTERNAL_H
