// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the types and functions your application uses to be notified of Ticos
 * SDK client library log messages.
 *
 * @details If you define the `TI_NO_LOGGING` symbol when compiling the SDK code (or adding option
 * `-DLOGGING=OFF` with cmake), all of the Ticos SDK logging functionality will be excluded, making
 * the resulting compiled code smaller and faster.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_LOG_H
#define _ti_LOG_H

#include <ti_result.h>
#include <ti_span.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

/**
 * @brief Identifies the classifications of log messages produced by the SDK.
 *
 * @note See the following `ti_log_classification` values from various headers:
 * - #ti_log_classification_core
 * - #ti_log_classification_iot
 */
typedef int32_t ti_log_classification;

// ti_log_classification Bits:
//   - 31 Always 0.
//   - 16..30 Facility.
//   - 0..15 Code.

#define _ti_LOG_MAKE_CLASSIFICATION(facility, code) \
  ((ti_log_classification)(((uint32_t)(facility) << 16U) | (uint32_t)(code)))

/**
 * @brief Identifies the #ti_log_classification produced by the SDK Core.
 */
enum ti_log_classification_core
{
  TI_LOG_HTTP_REQUEST
  = _ti_LOG_MAKE_CLASSIFICATION(_ti_FACILITY_CORE_HTTP, 1), ///< HTTP request is about to be sent.

  TI_LOG_HTTP_RESPONSE
  = _ti_LOG_MAKE_CLASSIFICATION(_ti_FACILITY_CORE_HTTP, 2), ///< HTTP response was received.

  TI_LOG_HTTP_RETRY = _ti_LOG_MAKE_CLASSIFICATION(
      _ti_FACILITY_CORE_HTTP,
      3), ///< First HTTP request did not succeed and will be retried.
};

/**
 * @brief Defines the signature of the callback function that application developers must provide to
 * receive Ticos SDK log messages.
 *
 * @param[in] classification The log message's #ti_log_classification.
 * @param[in] message The log message.
 */
typedef void (*ti_log_message_fn)(ti_log_classification classification, ti_span message);

/**
 * @brief Defines the signature of the callback function that application developers must provide
 * which will be used to check whether a particular log classification should be logged.
 *
 * @param[in] classification The log message's #ti_log_classification.
 *
 * @return Whether or not a log message with the provided classification should be logged.
 */
typedef bool (*ti_log_classification_filter_fn)(ti_log_classification classification);

/**
 * @brief Sets the functions that will be invoked to report an SDK log message.
 *
 * @param[in] log_message_callback __[nullable]__ A pointer to the function that will be invoked
 * when the SDK reports a log message that should be logged according to the result of the
 * #ti_log_classification_filter_fn provided to #ti_log_set_classification_filter_callback(). If
 * `NULL`, no function will be invoked.
 *
 * @remarks By default, this is `NULL`, which means, no function is invoked.
 */
#ifndef TI_NO_LOGGING
void ti_log_set_message_callback(ti_log_message_fn log_message_callback);
#else
TI_INLINE void ti_log_set_message_callback(ti_log_message_fn log_message_callback)
{
  (void)log_message_callback;
}
#endif // TI_NO_LOGGING

/**
 * @brief Sets the functions that will be invoked to check whether an SDK log message should be
 * reported.
 *
 * @param[in] message_filter_callback __[nullable]__ A pointer to the function that will be invoked
 * when the SDK checks whether a log message of a particular #ti_log_classification should be
 * logged. If `NULL`, log messages for all classifications will be logged, by passing them to the
 * #ti_log_message_fn provided to #ti_log_set_message_callback().
 *
 * @remarks By default, this is `NULL`, in which case no function is invoked to check whether a
 * classification should be logged or not. The SDK assumes true, passing messages with any log
 * classification to the #ti_log_message_fn provided to #ti_log_set_message_callback().
 */
#ifndef TI_NO_LOGGING
void ti_log_set_classification_filter_callback(
    ti_log_classification_filter_fn message_filter_callback);
#else
TI_INLINE void ti_log_set_classification_filter_callback(
    ti_log_classification_filter_fn message_filter_callback)
{
  (void)message_filter_callback;
}
#endif // TI_NO_LOGGING

#include <_ti_cfg_suffix.h>

#endif // _ti_LOG_H
