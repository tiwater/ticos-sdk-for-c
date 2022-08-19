// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines internals used by log.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_LOG_INTERNAL_H
#define _ti_LOG_INTERNAL_H

#include <ti_log.h>
#include <ti_span.h>

#include <stdbool.h>

#include <_ti_cfg_prefix.h>

#ifndef TI_NO_LOGGING

bool _ti_log_should_write(ti_log_classification classification);
void _ti_log_write(ti_log_classification classification, ti_span message);

#define _ti_LOG_SHOULD_WRITE(classification) _ti_log_should_write(classification)
#define _ti_LOG_WRITE(classification, message) _ti_log_write(classification, message)

#else

#define _ti_LOG_SHOULD_WRITE(classification) false

#define _ti_LOG_WRITE(classification, message)

#endif // TI_NO_LOGGING

#include <_ti_cfg_suffix.h>

#endif // _ti_LOG_INTERNAL_H
