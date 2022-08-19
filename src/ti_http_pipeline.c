// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_http.h>
#include <ti_http_internal.h>
#include <ti_precondition_internal.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_http_pipeline_process(
    _ti_http_pipeline* ref_pipeline,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  _ti_PRECONDITION_NOT_NULL(ref_request);
  _ti_PRECONDITION_NOT_NULL(ref_response);
  _ti_PRECONDITION_NOT_NULL(ref_pipeline);

  return ref_pipeline->_internal.policies[0]._internal.process(
      &(ref_pipeline->_internal.policies[1]),
      ref_pipeline->_internal.policies[0]._internal.options,
      ref_request,
      ref_response);
}
