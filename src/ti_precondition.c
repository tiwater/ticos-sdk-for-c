// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_precondition_internal.h>
#include <stdint.h>

#include <_ti_cfg.h>

static void ti_precondition_failed_default()
{
  /* By default, when a precondition fails the calling thread spins forever */
  while (1)
  {
  }
}

ti_precondition_failed_fn _ti_precondition_failed_callback = ti_precondition_failed_default;

void ti_precondition_failed_set_callback(ti_precondition_failed_fn ti_precondition_failed_callback)
{
  _ti_precondition_failed_callback = ti_precondition_failed_callback;
}

ti_precondition_failed_fn ti_precondition_failed_get_callback()
{
  return _ti_precondition_failed_callback;
}
