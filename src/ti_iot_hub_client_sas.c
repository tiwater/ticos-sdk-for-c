// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_precondition.h>
#include <ti_span.h>
#include <ti_result_internal.h>
#include <ti_iot_hub_client.h>
#include <ti_iot_common_internal.h>

#include <ti_log_internal.h>
#include <ti_precondition_internal.h>
#include <ti_span_internal.h>

#include <stdint.h>

#include <_ti_cfg.h>

#define LF '\n'
#define AMPERSAND '&'
#define EQUAL_SIGN '='
#define STRING_NULL_TERMINATOR '\0'
#define SCOPE_DEVICES_STRING "%2Fdevices%2F"
#define SCOPE_MODULES_STRING "%2Fmodules%2F"
#define SAS_TOKEN_SR "SharedAccessSignature sr"
#define SAS_TOKEN_SE "se"
#define SAS_TOKEN_SIG "sig"
#define SAS_TOKEN_SKN "skn"

static const ti_span devices_string = TI_SPAN_LITERAL_FROM_STR(SCOPE_DEVICES_STRING);
static const ti_span modules_string = TI_SPAN_LITERAL_FROM_STR(SCOPE_MODULES_STRING);
static const ti_span skn_string = TI_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SKN);
static const ti_span sr_string = TI_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SR);
static const ti_span sig_string = TI_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SIG);
static const ti_span se_string = TI_SPAN_LITERAL_FROM_STR(SAS_TOKEN_SE);

TI_NODISCARD ti_result ti_iot_hub_client_sas_get_signature(
    ti_iot_hub_client const* client,
    uint64_t token_expiration_epoch_time,
    ti_span signature,
    ti_span* out_signature)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION(token_expiration_epoch_time > 0);
  _ti_PRECONDITION_VALID_SPAN(signature, 1, false);
  _ti_PRECONDITION_NOT_NULL(out_signature);

  ti_span remainder = signature;
  int32_t signature_size = ti_span_size(signature);

  _ti_RETURN_IF_FAILED(
      _ti_span_copy_url_encode(remainder, client->_internal.iot_hub_hostname, &remainder));

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(devices_string));
  remainder = ti_span_copy(remainder, devices_string);

  _ti_RETURN_IF_FAILED(
      _ti_span_copy_url_encode(remainder, client->_internal.device_id, &remainder));

  if (ti_span_size(client->_internal.options.module_id) > 0)
  {
    _ti_RETURN_IF_NOT_ENOUGH_SIZE(remainder, ti_span_size(modules_string));
    remainder = ti_span_copy(remainder, modules_string);

    _ti_RETURN_IF_FAILED(
        _ti_span_copy_url_encode(remainder, client->_internal.options.module_id, &remainder));
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      remainder,
      1 + // LF
          _ti_iot_u64toa_size(token_expiration_epoch_time));

  remainder = ti_span_copy_u8(remainder, LF);

  _ti_RETURN_IF_FAILED(ti_span_u64toa(remainder, token_expiration_epoch_time, &remainder));

  *out_signature = ti_span_slice(signature, 0, signature_size - ti_span_size(remainder));
  _ti_LOG_WRITE(TI_LOG_IOT_SAS_TOKEN, *out_signature);

  return TI_OK;
}

TI_NODISCARD ti_result ti_iot_hub_client_sas_get_password(
    ti_iot_hub_client const* client,
    uint64_t token_expiration_epoch_time,
    ti_span base64_hmac_sha256_signature,
    ti_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length)
{
  _ti_PRECONDITION_NOT_NULL(client);
  _ti_PRECONDITION_VALID_SPAN(base64_hmac_sha256_signature, 1, false);
  _ti_PRECONDITION(token_expiration_epoch_time > 0);
  _ti_PRECONDITION_NOT_NULL(mqtt_password);
  _ti_PRECONDITION(mqtt_password_size > 0);

  // Concatenates: "SharedAccessSignature sr=" scope "&sig=" sig  "&se=" expiration_time_secs
  //               plus, if key_name size > 0, "&skn=" key_name

  ti_span mqtt_password_span = ti_span_create((uint8_t*)mqtt_password, (int32_t)mqtt_password_size);

  // SharedAccessSignature
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_password_span, ti_span_size(sr_string) + 1 /* EQUAL_SIGN */);
  mqtt_password_span = ti_span_copy(mqtt_password_span, sr_string);
  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, EQUAL_SIGN);

  _ti_RETURN_IF_FAILED(_ti_span_copy_url_encode(
      mqtt_password_span, client->_internal.iot_hub_hostname, &mqtt_password_span));

  // Device ID
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_password_span, ti_span_size(devices_string));
  mqtt_password_span = ti_span_copy(mqtt_password_span, devices_string);

  _ti_RETURN_IF_FAILED(_ti_span_copy_url_encode(
      mqtt_password_span, client->_internal.device_id, &mqtt_password_span));

  // Module ID
  if (ti_span_size(client->_internal.options.module_id) > 0)
  {
    _ti_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_password_span, ti_span_size(modules_string));
    mqtt_password_span = ti_span_copy(mqtt_password_span, modules_string);

    _ti_RETURN_IF_FAILED(_ti_span_copy_url_encode(
        mqtt_password_span, client->_internal.options.module_id, &mqtt_password_span));
  }

  // Signature
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_password_span, 1 /* AMPERSAND */ + ti_span_size(sig_string) + 1 /* EQUAL_SIGN */);

  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, AMPERSAND);
  mqtt_password_span = ti_span_copy(mqtt_password_span, sig_string);
  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, EQUAL_SIGN);

  _ti_RETURN_IF_FAILED(_ti_span_copy_url_encode(
      mqtt_password_span, base64_hmac_sha256_signature, &mqtt_password_span));

  // Expiration
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(
      mqtt_password_span, 1 /* AMPERSAND */ + ti_span_size(se_string) + 1 /* EQUAL_SIGN */);
  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, AMPERSAND);
  mqtt_password_span = ti_span_copy(mqtt_password_span, se_string);
  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, EQUAL_SIGN);

  _ti_RETURN_IF_FAILED(
      ti_span_u64toa(mqtt_password_span, token_expiration_epoch_time, &mqtt_password_span));

  if (ti_span_size(key_name) > 0)
  {
    // Key Name
    _ti_RETURN_IF_NOT_ENOUGH_SIZE(
        mqtt_password_span,
        1 /* AMPERSAND */ + ti_span_size(skn_string) + 1 /* EQUAL_SIGN */ + ti_span_size(key_name));
    mqtt_password_span = ti_span_copy_u8(mqtt_password_span, AMPERSAND);
    mqtt_password_span = ti_span_copy(mqtt_password_span, skn_string);
    mqtt_password_span = ti_span_copy_u8(mqtt_password_span, EQUAL_SIGN);
    mqtt_password_span = ti_span_copy(mqtt_password_span, key_name);
  }

  _ti_RETURN_IF_NOT_ENOUGH_SIZE(mqtt_password_span, 1 /* NULL TERMINATOR */);

  mqtt_password_span = ti_span_copy_u8(mqtt_password_span, STRING_NULL_TERMINATOR);

  if (out_mqtt_password_length != NULL)
  {
    *out_mqtt_password_length
        = mqtt_password_size - (size_t)ti_span_size(mqtt_password_span) - 1 /* NULL TERMINATOR */;
  }

  return TI_OK;
}
