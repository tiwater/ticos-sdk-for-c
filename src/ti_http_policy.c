// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_http_private.h"
#include <ti_credentials.h>
#include <ti_http.h>
#include <ti_precondition.h>
#include <ti_span.h>
#include <ti_version.h>
#include <ti_http_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_http_pipeline_policy_apiversion(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{

  _ti_http_policy_apiversion_options const* const options
      = (_ti_http_policy_apiversion_options const*)ref_options;

  switch (options->_internal.option_location)
  {
    case _ti_http_policy_apiversion_option_location_header:
      // Add the version as a header
      _ti_RETURN_IF_FAILED(ti_http_request_append_header(
          ref_request, options->_internal.name, options->_internal.version));
      break;
    case _ti_http_policy_apiversion_option_location_queryparameter:
      // Add the version as a query parameter. This value doesn't need url-encoding. Use `true` for
      // url-encode to avoid encoding.
      _ti_RETURN_IF_FAILED(ti_http_request_set_query_parameter(
          ref_request, options->_internal.name, options->_internal.version, true));
      break;
    default:
      return TI_ERROR_ARG;
  }

  return _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
}

// "-1" below is to account for the null terminator at the end of the string.
#define _ti_TELEMETRY_ID_PREFIX "tisdk-c-"
#define _ti_TELEMETRY_ID_PREFIX_LENGTH (sizeof(_ti_TELEMETRY_ID_PREFIX) - 1)
#define _ti_TELEMETRY_COMPONENT_NAME_MAX_LENGTH 40
#define _ti_TELEMETRY_VERSION_MAX_LENGTH (sizeof("12.345.6789-preview.123") - 1)
#define _ti_TELEMETRY_ID_MAX_LENGTH                                                       \
  (_ti_TELEMETRY_ID_PREFIX_LENGTH + _ti_TELEMETRY_COMPONENT_NAME_MAX_LENGTH + sizeof('/') \
   + _ti_TELEMETRY_VERSION_MAX_LENGTH)

TI_NODISCARD ti_result ti_http_pipeline_policy_telemetry(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  _ti_PRECONDITION_NOT_NULL(ref_options);

  // Format spec: https://tiwater.github.io/ticos-sdk/general_ticoscore.html#telemetry-policy
  uint8_t telemetry_id_buffer[_ti_TELEMETRY_ID_MAX_LENGTH] = _ti_TELEMETRY_ID_PREFIX;
  ti_span telemetry_id = TI_SPAN_FROM_BUFFER(telemetry_id_buffer);
  {
    ti_span remainder = ti_span_slice_to_end(telemetry_id, _ti_TELEMETRY_ID_PREFIX_LENGTH);

    _ti_http_policy_telemetry_options* options = (_ti_http_policy_telemetry_options*)(ref_options);
    ti_span const component_name = options->component_name;
#ifndef TI_NO_PRECONDITION_CHECKING
    {
      int32_t const component_name_size = ti_span_size(component_name);
      _ti_PRECONDITION_RANGE(1, component_name_size, _ti_TELEMETRY_COMPONENT_NAME_MAX_LENGTH);
    }
#endif // TI_NO_PRECONDITION_CHECKING
    remainder = ti_span_copy(remainder, component_name);

    remainder = ti_span_copy_u8(remainder, '/');
    remainder = ti_span_copy(remainder, TI_SPAN_FROM_STR(TI_SDK_VERSION_STRING));

    telemetry_id = ti_span_slice(telemetry_id, 0, _ti_span_diff(remainder, telemetry_id));
  }

  _ti_RETURN_IF_FAILED(
      ti_http_request_append_header(ref_request, TI_SPAN_FROM_STR("User-Agent"), telemetry_id));

  return _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
}

#undef _ti_TELEMETRY_ID_PREFIX
#undef _ti_TELEMETRY_ID_PREFIX_LENGTH
#undef _ti_TELEMETRY_COMPONENT_NAME_MAX_LENGTH
#undef _ti_TELEMETRY_VERSION_MAX_LENGTH
#undef _ti_TELEMETRY_ID_MAX_LENGTH

TI_NODISCARD ti_result ti_http_pipeline_policy_credential(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  _ti_credential* const credential = (_ti_credential*)ref_options;
  _ti_http_policy_process_fn const policy_credential_apply
      = credential == NULL ? NULL : credential->_internal.apply_credential_policy;

  if (credential == TI_CREDENTIAL_ANONYMOUS || policy_credential_apply == NULL)
  {
    return _ti_http_pipeline_nextpolicy(ref_policies, ref_request, ref_response);
  }

  return policy_credential_apply(ref_policies, credential, ref_request, ref_response);
}

TI_NODISCARD ti_result ti_http_pipeline_policy_transport(
    _ti_http_policy* ref_policies,
    void* ref_options,
    ti_http_request* ref_request,
    ti_http_response* ref_response)
{
  (void)ref_policies; // this is the last policy in the pipeline, we just void it
  (void)ref_options;

  // make sure the response is resetted
  _ti_http_response_reset(ref_response);

  return ti_http_client_send_request(ref_request, ref_response);
}
