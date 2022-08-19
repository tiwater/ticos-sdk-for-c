// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_http.h>

#include "ti_http_header_validation_private.h"
#include "ti_http_private.h"
#include "ti_span_private.h"
#include <ti_precondition.h>
#include <ti_precondition_internal.h>
#include <ti_result_internal.h>

#include <_ti_cfg.h>
#include <ctype.h>

// HTTP Response utility functions

static TI_NODISCARD bool _ti_is_http_whitespace(uint8_t c)
{
  switch (c)
  {
    case ' ':
    case '\t':
      return true;
      ;
    default:
      return false;
  }
}

/* PRIVATE Function. parse next  */
static TI_NODISCARD ti_result _ti_get_digit(ti_span* ref_span, uint8_t* save_here)
{

  if (ti_span_size(*ref_span) == 0)
  {
    return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
  }

  uint8_t c_ptr = ti_span_ptr(*ref_span)[0];
  if (!isdigit(c_ptr))
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }
  //
  *save_here = (uint8_t)(c_ptr - '0');

  // move reader after the expected digit (means it was parsed as expected)
  *ref_span = ti_span_slice_to_end(*ref_span, 1);

  return TI_OK;
}

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 * HTTP-version SP status-code SP reason-phrase CRLF
 */
static TI_NODISCARD ti_result
_ti_get_http_status_line(ti_span* ref_span, ti_http_response_status_line* out_status_line)
{

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6
  ti_span const start = TI_SPAN_FROM_STR("HTTP/");
  ti_span const dot = TI_SPAN_FROM_STR(".");
  ti_span const space = TI_SPAN_FROM_STR(" ");

  // parse and move reader if success
  _ti_RETURN_IF_FAILED(_ti_is_expected_span(ref_span, start));
  _ti_RETURN_IF_FAILED(_ti_get_digit(ref_span, &out_status_line->major_version));
  _ti_RETURN_IF_FAILED(_ti_is_expected_span(ref_span, dot));
  _ti_RETURN_IF_FAILED(_ti_get_digit(ref_span, &out_status_line->minor_version));

  // SP = " "
  _ti_RETURN_IF_FAILED(_ti_is_expected_span(ref_span, space));

  // status-code = 3DIGIT
  {
    uint64_t code = 0;
    _ti_RETURN_IF_FAILED(ti_span_atou64(ti_span_create(ti_span_ptr(*ref_span), 3), &code));
    out_status_line->status_code = (ti_http_status_code)code;
    // move reader
    *ref_span = ti_span_slice_to_end(*ref_span, 3);
  }

  // SP
  _ti_RETURN_IF_FAILED(_ti_is_expected_span(ref_span, space));

  // get a pointer to read response until end of reason-phrase is found
  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  int32_t offset = 0;
  int32_t input_size = ti_span_size(*ref_span);
  uint8_t const* const ptr = ti_span_ptr(*ref_span);
  for (; offset < input_size; ++offset)
  {
    uint8_t next_byte = ptr[offset];
    if (next_byte == '\n')
    {
      break;
    }
  }
  if (offset == input_size)
  {
    return TI_ERROR_ITEM_NOT_FOUND;
  }

  // save reason-phrase in status line now that we got the offset. Remove 1 last chars(\r)
  out_status_line->reason_phrase = ti_span_slice(*ref_span, 0, offset - 1);
  // move position of reader after reason-phrase (parsed done)
  *ref_span = ti_span_slice_to_end(*ref_span, offset + 1);
  // CR LF
  // _ti_RETURN_IF_FAILED(_ti_is_expected_span(response, TI_SPAN_FROM_STR("\r\n")));

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_response_get_status_line(
    ti_http_response* ref_response,
    ti_http_response_status_line* out_status_line)
{
  _ti_PRECONDITION_NOT_NULL(ref_response);
  _ti_PRECONDITION_NOT_NULL(out_status_line);

  // Restart parser to the beginning
  ref_response->_internal.parser.remaining = ref_response->_internal.http_response;

  // read an HTTP status line.
  _ti_RETURN_IF_FAILED(
      _ti_get_http_status_line(&ref_response->_internal.parser.remaining, out_status_line));

  // set state.kind of the next HTTP response value.
  ref_response->_internal.parser.next_kind = _ti_HTTP_RESPONSE_KIND_HEADER;

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_response_get_next_header(
    ti_http_response* ref_response,
    ti_span* out_name,
    ti_span* out_value)
{
  _ti_PRECONDITION_NOT_NULL(ref_response);
  _ti_PRECONDITION_NOT_NULL(out_name);
  _ti_PRECONDITION_NOT_NULL(out_value);

  ti_span* reader = &ref_response->_internal.parser.remaining;
  {
    _ti_http_response_kind const kind = ref_response->_internal.parser.next_kind;
    // if reader is expecting to read body (all headers were read), return
    // TI_ERROR_HTTP_END_OF_HEADERS so we know we reach end of headers
    if (kind == _ti_HTTP_RESPONSE_KIND_BODY)
    {
      return TI_ERROR_HTTP_END_OF_HEADERS;
    }
    // Can't read a header if status line was not previously called,
    // User needs to call ti_http_response_status_line() which would reset parser and set kind to
    // headers
    if (kind != _ti_HTTP_RESPONSE_KIND_HEADER)
    {
      return TI_ERROR_HTTP_INVALID_STATE;
    }
  }

  if (ti_span_size(ref_response->_internal.parser.remaining) == 0)
  {
    // avoid reading address if span is size 0
    return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
  }

  // check if we are at the end of all headers to change state to Body.
  // We keep state to Headers if current char is not '\r' (there is another header)
  if (ti_span_ptr(ref_response->_internal.parser.remaining)[0] == '\r')
  {
    _ti_RETURN_IF_FAILED(_ti_is_expected_span(reader, TI_SPAN_FROM_STR("\r\n")));
    ref_response->_internal.parser.next_kind = _ti_HTTP_RESPONSE_KIND_BODY;
    return TI_ERROR_HTTP_END_OF_HEADERS;
  }

  // https://tools.ietf.org/html/rfc7230#section-3.2
  // header-field   = field-name ":" OWS field-value OWS
  // field-name     = token
  {
    // https://tools.ietf.org/html/rfc7230#section-3.2.6
    // token = 1*tchar
    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" /
    //         "_" / "`" / "|" / "~" / DIGIT / ALPHA;
    // any VCHAR,
    //    except delimiters
    int32_t field_name_length = 0;
    int32_t input_size = ti_span_size(*reader);
    uint8_t const* const ptr = ti_span_ptr(*reader);
    for (; field_name_length < input_size; ++field_name_length)
    {
      uint8_t next_byte = ptr[field_name_length];
      if (next_byte == ':')
      {
        break;
      }
      if (!ti_http_valid_token[next_byte])
      {
        return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
      }
    }
    if (field_name_length == input_size)
    {
      return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
    }

    // form a header name. Reader is currently at char ':'
    *out_name = ti_span_slice(*reader, 0, field_name_length);
    // update reader to next position after colon (add one)
    *reader = ti_span_slice_to_end(*reader, field_name_length + 1);

    // Remove whitespace characters from header name
    // https://github.com/tiwater/ticos-sdk-for-c/issues/604
    *out_name = _ti_span_trim_whitespace(*out_name);

    // OWS -> remove the optional whitespace characters before header value
    int32_t ows_len = 0;
    input_size = ti_span_size(*reader);
    uint8_t const* const ptr_space = ti_span_ptr(*reader);
    for (; ows_len < input_size; ++ows_len)
    {
      uint8_t next_byte = ptr_space[ows_len];
      if (next_byte != ' ' && next_byte != '\t')
      {
        break;
      }
    }
    if (ows_len == input_size)
    {
      return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
    }

    *reader = ti_span_slice_to_end(*reader, ows_len);
  }
  // field-value    = *( field-content / obs-fold )
  // field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
  // field-vchar    = VCHAR / obs-text
  //
  // obs-fold       = CRLF 1*( SP / HTAB )
  //                ; obsolete line folding
  //                ; see Section 3.2.4
  //
  // Note: obs-fold is not implemented.
  {
    int32_t offset = 0;
    int32_t offset_value_end = offset;
    while (true)
    {
      uint8_t c = ti_span_ptr(*reader)[offset];
      offset += 1;
      if (c == '\r')
      {
        break; // break as soon as end of value char is found
      }
      if (_ti_is_http_whitespace(c))
      {
        continue; // whitespace or tab is accepted. It can be any number after value (OWS)
      }
      if (c < ' ')
      {
        return TI_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
      }
      offset_value_end = offset; // increasing index only for valid chars,
    }
    *out_value = ti_span_slice(*reader, 0, offset_value_end);
    // moving reader. It is currently after \r was found
    *reader = ti_span_slice_to_end(*reader, offset);

    // Remove whitespace characters from value https://github.com/tiwater/ticos-sdk-for-c/issues/604
    *out_value = _ti_span_trim_whitespace_from_end(*out_value);
  }

  _ti_RETURN_IF_FAILED(_ti_is_expected_span(reader, TI_SPAN_FROM_STR("\n")));

  return TI_OK;
}

TI_NODISCARD ti_result ti_http_response_get_body(ti_http_response* ref_response, ti_span* out_body)
{
  _ti_PRECONDITION_NOT_NULL(ref_response);
  _ti_PRECONDITION_NOT_NULL(out_body);

  // Make sure get body works no matter where is the current parsing. Allow users to call get body
  // directly and ignore headers and status line
  _ti_http_response_kind current_parsing_section = ref_response->_internal.parser.next_kind;
  if (current_parsing_section != _ti_HTTP_RESPONSE_KIND_BODY)
  {
    if (current_parsing_section == _ti_HTTP_RESPONSE_KIND_EOF
        || current_parsing_section == _ti_HTTP_RESPONSE_KIND_STATUS_LINE)
    {
      // Reset parser and get status line
      ti_http_response_status_line ignore = { 0 };
      _ti_RETURN_IF_FAILED(ti_http_response_get_status_line(ref_response, &ignore));
      // update current parsing section
      current_parsing_section = ref_response->_internal.parser.next_kind;
    }
    // parse any remaining header
    if (current_parsing_section == _ti_HTTP_RESPONSE_KIND_HEADER)
    {
      // Parse and ignore all remaining headers
      for (ti_span n = { 0 }, v = { 0 };
           ti_result_succeeded(ti_http_response_get_next_header(ref_response, &n, &v));)
      {
        // ignoring header
      }
    }
  }

  // take all the remaining content from reader as body
  *out_body = ti_span_slice_to_end(ref_response->_internal.parser.remaining, 0);

  ref_response->_internal.parser.next_kind = _ti_HTTP_RESPONSE_KIND_EOF;
  return TI_OK;
}

void _ti_http_response_reset(ti_http_response* ref_response)
{
  // never fails, discard the result
  // init will set written to 0 and will use the same ti_span. Internal parser's state is also
  // reset
  ti_result result = ti_http_response_init(ref_response, ref_response->_internal.http_response);
  (void)result;
}

// internal function to get ti_http_response remainder
static ti_span _ti_http_response_get_remaining(ti_http_response const* response)
{
  return ti_span_slice_to_end(response->_internal.http_response, response->_internal.written);
}

TI_NODISCARD ti_result ti_http_response_append(ti_http_response* ref_response, ti_span source)
{
  _ti_PRECONDITION_NOT_NULL(ref_response);

  ti_span remaining = _ti_http_response_get_remaining(ref_response);
  int32_t write_size = ti_span_size(source);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining, write_size);

  ti_span_copy(remaining, source);
  ref_response->_internal.written += write_size;

  return TI_OK;
}
