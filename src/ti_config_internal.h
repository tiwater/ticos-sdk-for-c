// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief 定义内部使用的常量
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Ticos SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _ti_CONFIG_INTERNAL_H
#define _ti_CONFIG_INTERNAL_H

#include <ti_config.h>
#include <ti_span.h>

#include <stdint.h>

#include <_ti_cfg_prefix.h>

enum
{
  _ti_TIME_SECONDS_PER_MINUTE = 60,
  _ti_TIME_MILLISECONDS_PER_SECOND = 1000,
  _ti_TIME_MICROSECONDS_PER_MILLISECOND = 1000,
};

/*
 *  Int64 is max value 9223372036854775808   (19 characters)
 *           min value -9223372036854775808  (20 characters)
 */
enum
{
  _ti_INT64_AS_STR_BUFFER_SIZE = 20,
};

#include <_ti_cfg_suffix.h>

#endif // _ti_CONFIG_INTERNAL_H
