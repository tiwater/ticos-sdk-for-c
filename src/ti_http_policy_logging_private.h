// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines private implementation used by http logging.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_HTTP_POLICY_LOGGING_PRIVATE_H
#define _ti_HTTP_POLICY_LOGGING_PRIVATE_H

#include <ti_http.h>
#include <ti_http_transport.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

void _ti_http_policy_logging_log_http_request(ti_http_request const* request);

void _ti_http_policy_logging_log_http_response(
    ti_http_response const* response,
    int64_t duration_msec,
    ti_http_request const* request);

#include <_ti_cfg_suffix.h>

#endif // _ti_HTTP_POLICY_LOGGING_PRIVATE_H
