// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines APIs to convert between binary data and UTF-8 encoded text that is represented in
 * base 64.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_BASE64_H
#define _ti_BASE64_H

#include <ti_result.h>
#include <ti_span.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Encodes the span of binary data into UTF-8 encoded text represented as base 64.
 *
 * @param destination_base64_text The output #ti_span where the encoded base 64 text should be
 * copied to as a result of the operation.
 * @param[in] source_bytes The input #ti_span that contains binary data to be encoded.
 * @param[out] out_written A pointer to an `int32_t` that receives the number of bytes written into
 * the destination #ti_span. This can be used to slice the output for subsequent calls, if
 * necessary.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_NOT_ENOUGH_SPACE The \p destination_base64_text is not large enough to contain
 * the encoded bytes.
 */
TI_NODISCARD ti_result
ti_base64_encode(ti_span destination_base64_text, ti_span source_bytes, int32_t* out_written);

/**
 * @brief Returns the maximum length of the result if you were to encode an #ti_span of the
 * specified length which contained binary data.
 *
 * @param source_bytes_size The size of the span containing binary data.
 *
 * @return The maximum length of the result.
 */
TI_NODISCARD int32_t ti_base64_get_max_encoded_size(int32_t source_bytes_size);

/**
 * @brief Decodes the span of UTF-8 encoded text represented as base 64 into binary data.
 *
 * @param destination_bytes The output #ti_span where the decoded binary data should be copied to as
 * a result of the operation.
 * @param[in] source_base64_text The input #ti_span that contains the base 64 text to be decoded.
 * @param[out] out_written A pointer to an `int32_t` that receives the number of bytes written into
 * the destination #ti_span. This can be used to slice the output for subsequent calls, if
 * necessary.
 *
 * @return An #ti_result value indicating the result of the operation.
 * @retval #TI_OK Success.
 * @retval #TI_ERROR_NOT_ENOUGH_SPACE The \p destination_bytes is not large enough to contain
 * the decoded text.
 * @retval #TI_ERROR_UNEXPECTED_CHAR The input \p source_base64_text contains characters outside of
 * the expected base 64 range, has invalid or more than two padding characters, or is incomplete
 * (that is, not a multiple of 4).
 * @retval #TI_ERROR_UNEXPECTED_END The input \p source_base64_text is incomplete (that is, it is
 * not of a size which is a multiple of 4).
 */
TI_NODISCARD ti_result
ti_base64_decode(ti_span destination_bytes, ti_span source_base64_text, int32_t* out_written);

/**
 * @brief Returns the maximum length of the result if you were to decode an #ti_span of the
 * specified length which contained base 64 encoded text.
 *
 * @param source_base64_text_size The size of the span containing base 64 encoded text.
 *
 * @return The maximum length of the result.
 */
TI_NODISCARD int32_t ti_base64_get_max_decoded_size(int32_t source_base64_text_size);

#include <_ti_cfg_suffix.h>

#endif // _ti_BASE64_H
