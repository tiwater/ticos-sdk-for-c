// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_platform.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_platform_clock_msec(int64_t* out_clock_msec)
{
  _ti_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = 0;
  return TI_ERROR_DEPENDENCY_NOT_PROVIDED;
}

TI_NODISCARD ti_result ti_platform_sleep_msec(int32_t milliseconds)
{
  (void)milliseconds;
  return TI_ERROR_DEPENDENCY_NOT_PROVIDED;
}
