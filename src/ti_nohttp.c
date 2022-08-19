// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_http_transport.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result
ti_http_client_send_request(ti_http_request const* request, ti_http_response* ref_response)
{
  (void)request;
  (void)ref_response;
  return TI_ERROR_DEPENDENCY_NOT_PROVIDED;
}
