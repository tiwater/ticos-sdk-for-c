// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines internals used by span.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_SPAN_INTERNAL_H
#define _ti_SPAN_INTERNAL_H

#include <ti_result.h>
#include <ti_span.h>
#include <ti_precondition_internal.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

// The smallest number that has the same number of digits as _ti_MAX_SIZE_FOR_UINT64 (i.e. 10^19).
#define _ti_SMALLEST_20_DIGIT_NUMBER 10000000000000000000ULL

enum
{
  // For example: 2,147,483,648
  _ti_MAX_SIZE_FOR_UINT32 = 10,

  // For example: 18,446,744,073,709,551,615
  _ti_MAX_SIZE_FOR_UINT64 = 20,

  // The number of unique values in base 10 (decimal).
  _ti_NUMBER_OF_DECIMAL_VALUES = 10,

  // The smallest number that has the same number of digits as _ti_MAX_SIZE_FOR_UINT32 (i.e. 10^9).
  _ti_SMALLEST_10_DIGIT_NUMBER = 1000000000,
};

// Use this helper to figure out how much the sliced_span has moved in comparison to the
// original_span while writing and slicing a copy of the original.
// The \p sliced_span must be some slice of the \p original_span (and have the same backing memory).
TI_INLINE TI_NODISCARD int32_t _ti_span_diff(ti_span sliced_span, ti_span original_span)
{
  int32_t answer = ti_span_size(original_span) - ti_span_size(sliced_span);

  // The passed in span parameters cannot be any two arbitrary spans.
  // This validates the span parameters are valid and one is a sub-slice of another.
  _ti_PRECONDITION(answer == (int32_t)(ti_span_ptr(sliced_span) - ti_span_ptr(original_span)));
  return answer;
}

/**
 * @brief Copies character from the \p source #ti_span to the \p destination #ti_span by
 * URL-encoding the \p source span characters.
 *
 * @param destination The #ti_span whose bytes will receive the URL-encoded \p source.
 * @param[in] source The #ti_span containing the non-URL-encoded bytes.
 * @param[out] out_length A pointer to an int32_t that is going to be assigned the length
 * of URL-encoding the \p source.
 * @return An #ti_result value indicating the result of the operation:
 *         - #TI_OK if successful
 *         - #TI_ERROR_NOT_ENOUGH_SPACE if the \p destination is not big enough to contain the
 * encoded bytes
 *
 * @remark If \p destination can't fit the \p source, some data may still be written to it, but the
 * \p out_length will be set to 0, and the function will return #TI_ERROR_NOT_ENOUGH_SPACE.
 * @remark The \p destination and \p source must not overlap.
 */
TI_NODISCARD ti_result
_ti_span_url_encode(ti_span destination, ti_span source, int32_t* out_length);

/**
 * @brief Calculates what would be the length of \p source #ti_span after url-encoding it.
 *
 * @param[in] source The #ti_span containing the non-URL-encoded bytes.
 * @return The length of source if it would be url-encoded.
 *
 */
TI_NODISCARD int32_t _ti_span_url_encode_calc_length(ti_span source);

/**
 * @brief String tokenizer for #ti_span.
 *
 * @param[in] source The #ti_span with the content to be searched on. It must be a non-empty
 * #ti_span.
 * @param[in] delimiter The #ti_span containing the delimiter to "split" `source` into tokens.  It
 * must be a non-empty #ti_span.
 * @param[out] out_remainder The #ti_span pointing to the remaining bytes in `source`, starting
 * after the occurrence of `delimiter`. If the position after `delimiter` is the end of `source`,
 * `out_remainder` is set to an empty #ti_span.
 * @param[out] out_index The position of \p delimiter in \p source if \p source contains the \p
 * delimiter within it. Otherwise, it is set to -1.
 *
 * @return The #ti_span pointing to the token delimited by the beginning of `source` up to the first
 * occurrence of (but not including the) `delimiter`, or the end of `source` if `delimiter` is not
 * found. If `source` is empty, #TI_SPAN_EMPTY is returned instead.
 */
ti_span _ti_span_token(
    ti_span source,
    ti_span delimiter,
    ti_span* out_remainder,
    int32_t* out_index);

#include <_ti_cfg_suffix.h>

#endif // _ti_SPAN_INTERNAL_H
