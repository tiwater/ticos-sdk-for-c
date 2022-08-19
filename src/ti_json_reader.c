// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_json_private.h"
#include "ti_span_private.h"
#include <ti_precondition.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include <ctype.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_json_reader_init(
    ti_json_reader* out_json_reader,
    ti_span json_buffer,
    ti_json_reader_options const* options)
{
  _ti_PRECONDITION(ti_span_size(json_buffer) >= 1);

  *out_json_reader = (ti_json_reader){
    .token = (ti_json_token){
      .kind = TI_JSON_TOKEN_NONE,
      .slice = TI_SPAN_EMPTY,
      .size = 0,
      ._internal = {
        .is_multisegment = false,
        .string_has_escaped_chars = false,
        .pointer_to_first_buffer = &TI_SPAN_EMPTY,
        .start_buffer_index = -1,
        .start_buffer_offset = -1,
        .end_buffer_index = -1,
        .end_buffer_offset = -1,
      },
    },
    .current_depth = 0,
    ._internal = {
      .json_buffer = json_buffer,
      .json_buffers = &TI_SPAN_EMPTY,
      .number_of_buffers = 1,
      .buffer_index = 0,
      .bytes_consumed = 0,
      .total_bytes_consumed = 0,
      .is_complex_json = false,
      .bit_stack = { 0 },
      .options = options == NULL ? ti_json_reader_options_default() : *options,
    },
  };
  return TI_OK;
}

TI_NODISCARD ti_result ti_json_reader_chunked_init(
    ti_json_reader* out_json_reader,
    ti_span json_buffers[],
    int32_t number_of_buffers,
    ti_json_reader_options const* options)
{
  _ti_PRECONDITION(number_of_buffers >= 1);
  _ti_PRECONDITION(ti_span_size(json_buffers[0]) >= 1);

  *out_json_reader = (ti_json_reader)
  {
    .token = (ti_json_token){
      .kind = TI_JSON_TOKEN_NONE,
      .slice = TI_SPAN_EMPTY,
      .size = 0,
      ._internal = {
        .is_multisegment = false,
        .string_has_escaped_chars = false,
        .pointer_to_first_buffer = json_buffers,
        .start_buffer_index = -1,
        .start_buffer_offset = -1,
        .end_buffer_index = -1,
        .end_buffer_offset = -1,
      },
    },
    .current_depth = 0,
    ._internal = {
      .json_buffer = json_buffers[0],
      .json_buffers = json_buffers,
      .number_of_buffers = number_of_buffers,
      .buffer_index = 0,
      .bytes_consumed = 0,
      .total_bytes_consumed = 0,
      .is_complex_json = false,
      .bit_stack = { 0 },
      .options = options == NULL ? ti_json_reader_options_default() : *options,
    },
  };
  return TI_OK;
}

TI_NODISCARD static ti_span _get_remaining_json(ti_json_reader* json_reader)
{
  _ti_PRECONDITION_NOT_NULL(json_reader);

  return ti_span_slice_to_end(
      json_reader->_internal.json_buffer, json_reader->_internal.bytes_consumed);
}

static void _ti_json_reader_update_state(
    ti_json_reader* ref_json_reader,
    ti_json_token_kind token_kind,
    ti_span token_slice,
    int32_t current_segment_consumed,
    int32_t consumed)
{
  ref_json_reader->token.kind = token_kind;
  ref_json_reader->token.size = consumed;
  ref_json_reader->current_depth = ref_json_reader->_internal.bit_stack._internal.current_depth;

  // The depth of the start of the container will be one less than the bit stack managing the state.
  // That is because we push on the stack when we see a start of the container (above in the call
  // stack), but its actual depth and "indentation" level is one lower.
  if (token_kind == TI_JSON_TOKEN_BEGIN_ARRAY || token_kind == TI_JSON_TOKEN_BEGIN_OBJECT)
  {
    ref_json_reader->current_depth--;
  }

  ref_json_reader->_internal.bytes_consumed += current_segment_consumed;
  ref_json_reader->_internal.total_bytes_consumed += consumed;

  // We should have already set start_buffer_index and offset before moving to the next buffer.
  ref_json_reader->token._internal.end_buffer_index = ref_json_reader->_internal.buffer_index;
  ref_json_reader->token._internal.end_buffer_offset = ref_json_reader->_internal.bytes_consumed;

  ref_json_reader->token._internal.is_multisegment = false;

  // Token straddles more than one segment
  int32_t start_index = ref_json_reader->token._internal.start_buffer_index;
  if (start_index != -1 && start_index < ref_json_reader->token._internal.end_buffer_index)
  {
    ref_json_reader->token._internal.is_multisegment = true;
  }

  ref_json_reader->token.slice = token_slice;
}

TI_NODISCARD static ti_result _ti_json_reader_get_next_buffer(
    ti_json_reader* ref_json_reader,
    ti_span* remaining,
    bool skip_whitespace)
{
  // If we only had one buffer, or we ran out of the set of discontiguous buffers, return error.
  if (ref_json_reader->_internal.buffer_index >= ref_json_reader->_internal.number_of_buffers - 1)
  {
    return TI_ERROR_UNEXPECTED_END;
  }

  if (!skip_whitespace && ref_json_reader->token._internal.start_buffer_index == -1)
  {
    ref_json_reader->token._internal.start_buffer_index = ref_json_reader->_internal.buffer_index;

    ref_json_reader->token._internal.start_buffer_offset
        = ref_json_reader->_internal.bytes_consumed;
  }

  ref_json_reader->_internal.buffer_index++;

  ref_json_reader->_internal.json_buffer
      = ref_json_reader->_internal.json_buffers[ref_json_reader->_internal.buffer_index];

  ref_json_reader->_internal.bytes_consumed = 0;

  ti_span place_holder = _get_remaining_json(ref_json_reader);

  // Found an empty segment in the json_buffers array, which isn't allowed.
  if (ti_span_size(place_holder) < 1)
  {
    return TI_ERROR_UNEXPECTED_END;
  }

  *remaining = place_holder;
  return TI_OK;
}

TI_NODISCARD static ti_span _ti_json_reader_skip_whitespace(ti_json_reader* ref_json_reader)
{
  ti_span json;
  ti_span remaining = _get_remaining_json(ref_json_reader);

  while (true)
  {
    json = _ti_span_trim_whitespace_from_start(remaining);

    // Find out how many whitespace characters were trimmed.
    int32_t consumed = _ti_span_diff(json, remaining);

    ref_json_reader->_internal.bytes_consumed += consumed;
    ref_json_reader->_internal.total_bytes_consumed += consumed;

    if (ti_span_size(json) >= 1
        || ti_result_failed(_ti_json_reader_get_next_buffer(ref_json_reader, &remaining, true)))
    {
      break;
    }
  }

  return json;
}

TI_NODISCARD static ti_result _ti_json_reader_process_container_end(
    ti_json_reader* ref_json_reader,
    ti_json_token_kind token_kind)
{
  // The JSON payload is invalid if it has a mismatched container end without a matching open.
  if ((token_kind == TI_JSON_TOKEN_END_OBJECT
       && _ti_json_stack_peek(&ref_json_reader->_internal.bit_stack) != _ti_JSON_STACK_OBJECT)
      || (token_kind == TI_JSON_TOKEN_END_ARRAY
          && _ti_json_stack_peek(&ref_json_reader->_internal.bit_stack) != _ti_JSON_STACK_ARRAY))
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  ti_span token = _get_remaining_json(ref_json_reader);
  _ti_json_stack_pop(&ref_json_reader->_internal.bit_stack);
  _ti_json_reader_update_state(ref_json_reader, token_kind, ti_span_slice(token, 0, 1), 1, 1);
  return TI_OK;
}

TI_NODISCARD static ti_result _ti_json_reader_process_container_start(
    ti_json_reader* ref_json_reader,
    ti_json_token_kind token_kind,
    _ti_json_stack_item container_kind)
{
  // The current depth is equal to or larger than the maximum allowed depth of 64. Cannot read the
  // next JSON object or array.
  if (ref_json_reader->_internal.bit_stack._internal.current_depth >= _ti_MAX_JSON_STACK_SIZE)
  {
    return TI_ERROR_JSON_NESTING_OVERFLOW;
  }

  ti_span token = _get_remaining_json(ref_json_reader);

  _ti_json_stack_push(&ref_json_reader->_internal.bit_stack, container_kind);
  _ti_json_reader_update_state(ref_json_reader, token_kind, ti_span_slice(token, 0, 1), 1, 1);
  return TI_OK;
}

TI_NODISCARD static bool _ti_is_valid_escaped_character(uint8_t byte)
{
  switch (byte)
  {
    case '\\':
    case '"':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return true;
    default:
      return false;
  }
}

TI_NODISCARD static ti_result _ti_json_reader_process_string(ti_json_reader* ref_json_reader)
{
  // Move past the first '"' character
  ref_json_reader->_internal.bytes_consumed++;

  ti_span token = _get_remaining_json(ref_json_reader);
  int32_t remaining_size = ti_span_size(token);

  if (remaining_size < 1)
  {
    _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
    remaining_size = ti_span_size(token);
  }

  int32_t current_index = 0;
  int32_t string_length = 0;
  uint8_t* token_ptr = ti_span_ptr(token);
  uint8_t next_byte = token_ptr[0];

  // Clear the state of any previous string token.
  ref_json_reader->token._internal.string_has_escaped_chars = false;

  while (true)
  {
    if (next_byte == '"')
    {
      break;
    }

    if (next_byte == '\\')
    {
      ref_json_reader->token._internal.string_has_escaped_chars = true;
      current_index++;
      string_length++;
      if (current_index >= remaining_size)
      {
        _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
        current_index = 0;
        token_ptr = ti_span_ptr(token);
        remaining_size = ti_span_size(token);
      }
      next_byte = token_ptr[current_index];

      if (next_byte == 'u')
      {
        current_index++;
        string_length++;

        // Expecting 4 hex digits to follow the escaped 'u'
        for (int32_t i = 0; i < 4; i++)
        {
          if (current_index >= remaining_size)
          {
            _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
            current_index = 0;
            token_ptr = ti_span_ptr(token);
            remaining_size = ti_span_size(token);
          }

          string_length++;
          next_byte = token_ptr[current_index++];

          if (!isxdigit(next_byte))
          {
            return TI_ERROR_UNEXPECTED_CHAR;
          }
        }

        // We have already skipped past the u and 4 hex digits. The loop accounts for incrementing
        // by 1 more, so subtract one to account for that.
        current_index--;
        string_length--;
      }
      else
      {
        if (!_ti_is_valid_escaped_character(next_byte))
        {
          return TI_ERROR_UNEXPECTED_CHAR;
        }
      }
    }
    else
    {
      // Control characters are invalid within a JSON string and should be correctly escaped.
      if (next_byte < _ti_ASCII_SPACE_CHARACTER)
      {
        return TI_ERROR_UNEXPECTED_CHAR;
      }
    }

    current_index++;
    string_length++;

    if (current_index >= remaining_size)
    {
      _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
      current_index = 0;
      token_ptr = ti_span_ptr(token);
      remaining_size = ti_span_size(token);
    }
    next_byte = token_ptr[current_index];
  }

  _ti_json_reader_update_state(
      ref_json_reader,
      TI_JSON_TOKEN_STRING,
      ti_span_slice(token, 0, current_index),
      current_index,
      string_length);

  // Add 1 to number of bytes consumed to account for the last '"' character.
  ref_json_reader->_internal.bytes_consumed++;
  ref_json_reader->_internal.total_bytes_consumed++;

  return TI_OK;
}

TI_NODISCARD static ti_result _ti_json_reader_process_property_name(ti_json_reader* ref_json_reader)
{
  _ti_RETURN_IF_FAILED(_ti_json_reader_process_string(ref_json_reader));

  ti_span json = _ti_json_reader_skip_whitespace(ref_json_reader);

  // Expected a colon to indicate that a value will follow after the property name, but instead
  // either reached end of data or some other character, which is invalid.
  if (ti_span_size(json) < 1)
  {
    return TI_ERROR_UNEXPECTED_END;
  }
  if (ti_span_ptr(json)[0] != ':')
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // We don't need to set the json_reader->token.slice since that was already done
  // in _ti_json_reader_process_string when processing the string portion of the property name.
  // Therefore, we don't call _ti_json_reader_update_state here.
  ref_json_reader->token.kind = TI_JSON_TOKEN_PROPERTY_NAME;
  ref_json_reader->_internal.bytes_consumed++; // For the name / value separator
  ref_json_reader->_internal.total_bytes_consumed++; // For the name / value separator

  return TI_OK;
}

// Used to search for possible valid end of a number character, when we have complex JSON payloads
// (i.e. not a single JSON value).
// Whitespace characters, comma, or a container end character indicate the end of a JSON number.
static const ti_span json_delimiters = TI_SPAN_LITERAL_FROM_STR(",}] \n\r\t");

TI_NODISCARD static bool _ti_finished_consuming_json_number(
    uint8_t next_byte,
    ti_span expected_next_bytes,
    ti_result* out_result)
{
  ti_span next_byte_span = ti_span_create(&next_byte, 1);

  // Checking if we are done processing a JSON number
  int32_t index = ti_span_find(json_delimiters, next_byte_span);
  if (index != -1)
  {
    *out_result = TI_OK;
    return true;
  }

  // The next character after a "0" or a set of digits must either be a decimal or 'e'/'E' to
  // indicate scientific notation. For example "01" or "123f" is invalid.
  // The next character after "[-][digits].[digits]" must be 'e'/'E' if we haven't reached the end
  // of the number yet. For example, "1.1f" or "1.1-" are invalid.
  index = ti_span_find(expected_next_bytes, next_byte_span);
  if (index == -1)
  {
    *out_result = TI_ERROR_UNEXPECTED_CHAR;
    return true;
  }

  return false;
}

static void _ti_json_reader_consume_digits(
    ti_json_reader* ref_json_reader,
    ti_span* token,
    int32_t* current_consumed,
    int32_t* total_consumed)
{
  int32_t counter = 0;
  ti_span current = ti_span_slice_to_end(*token, *current_consumed);
  while (true)
  {
    int32_t const token_size = ti_span_size(current);
    uint8_t* next_byte_ptr = ti_span_ptr(current);

    while (counter < token_size)
    {
      if (isdigit(*next_byte_ptr))
      {
        counter++;
        next_byte_ptr++;
      }
      else
      {
        break;
      }
    }
    if (counter == token_size
        && ti_result_succeeded(_ti_json_reader_get_next_buffer(ref_json_reader, token, false)))
    {
      *total_consumed += counter;
      counter = 0;
      *current_consumed = 0;
      current = *token;
      continue;
    }
    break;
  }

  *total_consumed += counter;
  *current_consumed += counter;
}

TI_NODISCARD static ti_result _ti_json_reader_update_number_state_if_single_value(
    ti_json_reader* ref_json_reader,
    ti_span token_slice,
    int32_t current_consumed,
    int32_t total_consumed)
{
  if (ref_json_reader->_internal.is_complex_json)
  {
    return TI_ERROR_UNEXPECTED_END;
  }

  _ti_json_reader_update_state(
      ref_json_reader, TI_JSON_TOKEN_NUMBER, token_slice, current_consumed, total_consumed);

  return TI_OK;
}

TI_NODISCARD static ti_result _ti_validate_next_byte_is_digit(
    ti_json_reader* ref_json_reader,
    ti_span* remaining_number,
    int32_t* current_consumed)
{
  ti_span current = ti_span_slice_to_end(*remaining_number, *current_consumed);
  if (ti_span_size(current) < 1)
  {
    _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, remaining_number, false));
    current = *remaining_number;
    *current_consumed = 0;
  }

  if (!isdigit(ti_span_ptr(current)[0]))
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  return TI_OK;
}

TI_NODISCARD static ti_result _ti_json_reader_process_number(ti_json_reader* ref_json_reader)
{
  ti_span token = _get_remaining_json(ref_json_reader);

  int32_t total_consumed = 0;
  int32_t current_consumed = 0;

  uint8_t next_byte = ti_span_ptr(token)[0];
  if (next_byte == '-')
  {
    total_consumed++;
    current_consumed++;

    // A negative sign must be followed by at least one digit.
    _ti_RETURN_IF_FAILED(
        _ti_validate_next_byte_is_digit(ref_json_reader, &token, &current_consumed));

    next_byte = ti_span_ptr(token)[current_consumed];
  }

  if (next_byte == '0')
  {
    total_consumed++;
    current_consumed++;

    if (current_consumed >= ti_span_size(token))
    {
      if (ti_result_failed(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false)))
      {
        // If there is no more JSON, this is a valid end state only when the JSON payload contains a
        // single value: "[-]0"
        // Otherwise, the payload is incomplete and ending too early.
        return _ti_json_reader_update_number_state_if_single_value(
            ref_json_reader,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      current_consumed = 0;
    }

    next_byte = ti_span_ptr(token)[current_consumed];
    ti_result result = TI_OK;
    if (_ti_finished_consuming_json_number(next_byte, TI_SPAN_FROM_STR(".eE"), &result))
    {
      if (ti_result_succeeded(result))
      {
        _ti_json_reader_update_state(
            ref_json_reader,
            TI_JSON_TOKEN_NUMBER,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      return result;
    }
  }
  else
  {
    _ti_PRECONDITION(isdigit(next_byte));

    // Integer part before decimal
    _ti_json_reader_consume_digits(ref_json_reader, &token, &current_consumed, &total_consumed);

    if (current_consumed >= ti_span_size(token))
    {
      if (ti_result_failed(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false)))
      {
        // If there is no more JSON, this is a valid end state only when the JSON payload contains a
        // single value: "[-][digits]"
        // Otherwise, the payload is incomplete and ending too early.
        return _ti_json_reader_update_number_state_if_single_value(
            ref_json_reader,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      current_consumed = 0;
    }

    next_byte = ti_span_ptr(token)[current_consumed];
    ti_result result = TI_OK;
    if (_ti_finished_consuming_json_number(next_byte, TI_SPAN_FROM_STR(".eE"), &result))
    {
      if (ti_result_succeeded(result))
      {
        _ti_json_reader_update_state(
            ref_json_reader,
            TI_JSON_TOKEN_NUMBER,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      return result;
    }
  }

  if (next_byte == '.')
  {
    total_consumed++;
    current_consumed++;

    // A decimal point must be followed by at least one digit.
    _ti_RETURN_IF_FAILED(
        _ti_validate_next_byte_is_digit(ref_json_reader, &token, &current_consumed));

    // Integer part after decimal
    _ti_json_reader_consume_digits(ref_json_reader, &token, &current_consumed, &total_consumed);

    if (current_consumed >= ti_span_size(token))
    {
      if (ti_result_failed(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false)))
      {
        // If there is no more JSON, this is a valid end state only when the JSON payload contains a
        // single value: "[-][digits].[digits]"
        // Otherwise, the payload is incomplete and ending too early.
        return _ti_json_reader_update_number_state_if_single_value(
            ref_json_reader,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      current_consumed = 0;
    }

    next_byte = ti_span_ptr(token)[current_consumed];
    ti_result result = TI_OK;
    if (_ti_finished_consuming_json_number(next_byte, TI_SPAN_FROM_STR("eE"), &result))
    {
      if (ti_result_succeeded(result))
      {
        _ti_json_reader_update_state(
            ref_json_reader,
            TI_JSON_TOKEN_NUMBER,
            ti_span_slice(token, 0, current_consumed),
            current_consumed,
            total_consumed);
      }
      return result;
    }
  }

  // Move past 'e'/'E'
  total_consumed++;
  current_consumed++;

  // The 'e'/'E' character must be followed by a sign or at least one digit.
  if (current_consumed >= ti_span_size(token))
  {
    _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
    current_consumed = 0;
  }

  next_byte = ti_span_ptr(token)[current_consumed];
  if (next_byte == '-' || next_byte == '+')
  {
    total_consumed++;
    current_consumed++;

    // A sign must be followed by at least one digit.
    _ti_RETURN_IF_FAILED(
        _ti_validate_next_byte_is_digit(ref_json_reader, &token, &current_consumed));
  }

  // Integer part after the 'e'/'E'
  _ti_json_reader_consume_digits(ref_json_reader, &token, &current_consumed, &total_consumed);

  if (current_consumed >= ti_span_size(token))
  {
    if (ti_result_failed(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false)))
    {

      // If there is no more JSON, this is a valid end state only when the JSON payload contains a
      // single value: "[-][digits].[digits]e[+|-][digits]"
      // Otherwise, the payload is incomplete and ending too early.
      return _ti_json_reader_update_number_state_if_single_value(
          ref_json_reader,
          ti_span_slice(token, 0, current_consumed),
          current_consumed,
          total_consumed);
    }
    current_consumed = 0;
  }

  // Checking if we are done processing a JSON number
  next_byte = ti_span_ptr(token)[current_consumed];
  int32_t index = ti_span_find(json_delimiters, ti_span_create(&next_byte, 1));
  if (index == -1)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  _ti_json_reader_update_state(
      ref_json_reader,
      TI_JSON_TOKEN_NUMBER,
      ti_span_slice(token, 0, current_consumed),
      current_consumed,
      total_consumed);

  return TI_OK;
}

TI_INLINE int32_t _ti_min(int32_t a, int32_t b) { return a < b ? a : b; }

TI_NODISCARD static ti_result _ti_json_reader_process_literal(
    ti_json_reader* ref_json_reader,
    ti_span literal,
    ti_json_token_kind kind)
{
  ti_span token = _get_remaining_json(ref_json_reader);

  int32_t const expected_literal_size = ti_span_size(literal);

  int32_t already_matched = 0;

  int32_t max_comparable_size = 0;
  while (true)
  {
    int32_t token_size = ti_span_size(token);
    max_comparable_size = _ti_min(token_size, expected_literal_size - already_matched);

    token = ti_span_slice(token, 0, max_comparable_size);

    // Return if the subslice that can be compared contains a mismatch.
    if (!ti_span_is_content_equal(
            token, ti_span_slice(literal, already_matched, already_matched + max_comparable_size)))
    {
      return TI_ERROR_UNEXPECTED_CHAR;
    }
    already_matched += max_comparable_size;

    if (already_matched == expected_literal_size)
    {
      break;
    }

    // If there is no more data, return EOF because the token is smaller than the expected literal.
    _ti_RETURN_IF_FAILED(_ti_json_reader_get_next_buffer(ref_json_reader, &token, false));
  }

  _ti_json_reader_update_state(
      ref_json_reader, kind, token, max_comparable_size, expected_literal_size);
  return TI_OK;
}

TI_NODISCARD static ti_result _ti_json_reader_process_value(
    ti_json_reader* ref_json_reader,
    uint8_t const next_byte)
{
  if (next_byte == '"')
  {
    return _ti_json_reader_process_string(ref_json_reader);
  }

  if (next_byte == '{')
  {
    return _ti_json_reader_process_container_start(
        ref_json_reader, TI_JSON_TOKEN_BEGIN_OBJECT, _ti_JSON_STACK_OBJECT);
  }

  if (next_byte == '[')
  {
    return _ti_json_reader_process_container_start(
        ref_json_reader, TI_JSON_TOKEN_BEGIN_ARRAY, _ti_JSON_STACK_ARRAY);
  }

  if (isdigit(next_byte) || next_byte == '-')
  {
    return _ti_json_reader_process_number(ref_json_reader);
  }

  if (next_byte == 'f')
  {
    return _ti_json_reader_process_literal(
        ref_json_reader, TI_SPAN_FROM_STR("false"), TI_JSON_TOKEN_FALSE);
  }

  if (next_byte == 't')
  {
    return _ti_json_reader_process_literal(
        ref_json_reader, TI_SPAN_FROM_STR("true"), TI_JSON_TOKEN_TRUE);
  }

  if (next_byte == 'n')
  {
    return _ti_json_reader_process_literal(
        ref_json_reader, TI_SPAN_FROM_STR("null"), TI_JSON_TOKEN_NULL);
  }

  return TI_ERROR_UNEXPECTED_CHAR;
}

TI_NODISCARD static ti_result _ti_json_reader_read_first_token(
    ti_json_reader* ref_json_reader,
    ti_span json,
    uint8_t const first_byte)
{
  if (first_byte == '{')
  {
    _ti_json_stack_push(&ref_json_reader->_internal.bit_stack, _ti_JSON_STACK_OBJECT);

    _ti_json_reader_update_state(
        ref_json_reader, TI_JSON_TOKEN_BEGIN_OBJECT, ti_span_slice(json, 0, 1), 1, 1);

    ref_json_reader->_internal.is_complex_json = true;
    return TI_OK;
  }

  if (first_byte == '[')
  {
    _ti_json_stack_push(&ref_json_reader->_internal.bit_stack, _ti_JSON_STACK_ARRAY);

    _ti_json_reader_update_state(
        ref_json_reader, TI_JSON_TOKEN_BEGIN_ARRAY, ti_span_slice(json, 0, 1), 1, 1);

    ref_json_reader->_internal.is_complex_json = true;
    return TI_OK;
  }

  return _ti_json_reader_process_value(ref_json_reader, first_byte);
}

TI_NODISCARD static ti_result _ti_json_reader_process_next_byte(
    ti_json_reader* ref_json_reader,
    uint8_t next_byte)
{
  // Extra data after a single JSON value (complete object or array or one primitive value) is
  // invalid. Expected end of data.
  if (ref_json_reader->_internal.bit_stack._internal.current_depth == 0)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  bool within_object
      = _ti_json_stack_peek(&ref_json_reader->_internal.bit_stack) == _ti_JSON_STACK_OBJECT;

  if (next_byte == ',')
  {
    ref_json_reader->_internal.bytes_consumed++;

    ti_span json = _ti_json_reader_skip_whitespace(ref_json_reader);

    // Expected start of a property name or value, but instead reached end of data.
    if (ti_span_size(json) < 1)
    {
      return TI_ERROR_UNEXPECTED_END;
    }

    next_byte = ti_span_ptr(json)[0];

    if (within_object)
    {
      // Expected start of a property name after the comma since we are within a JSON object.
      if (next_byte != '"')
      {
        return TI_ERROR_UNEXPECTED_CHAR;
      }
      return _ti_json_reader_process_property_name(ref_json_reader);
    }

    return _ti_json_reader_process_value(ref_json_reader, next_byte);
  }

  if (next_byte == '}')
  {
    return _ti_json_reader_process_container_end(ref_json_reader, TI_JSON_TOKEN_END_OBJECT);
  }

  if (next_byte == ']')
  {
    return _ti_json_reader_process_container_end(ref_json_reader, TI_JSON_TOKEN_END_ARRAY);
  }

  // No other character is a valid token delimiter within JSON.
  return TI_ERROR_UNEXPECTED_CHAR;
}

TI_NODISCARD ti_result ti_json_reader_next_token(ti_json_reader* ref_json_reader)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_reader);

  ti_span json = _ti_json_reader_skip_whitespace(ref_json_reader);

  if (ti_span_size(json) < 1)
  {
    if (ref_json_reader->token.kind == TI_JSON_TOKEN_NONE
        || ref_json_reader->_internal.bit_stack._internal.current_depth != 0)
    {
      // An empty JSON payload is invalid.
      return TI_ERROR_UNEXPECTED_END;
    }

    // No more JSON text left to process, we are done.
    return TI_ERROR_JSON_READER_DONE;
  }

  // Clear the internal state of any previous token.
  ref_json_reader->token._internal.start_buffer_index = -1;
  ref_json_reader->token._internal.start_buffer_offset = -1;
  ref_json_reader->token._internal.end_buffer_index = -1;
  ref_json_reader->token._internal.end_buffer_offset = -1;

  uint8_t const first_byte = ti_span_ptr(json)[0];

  switch (ref_json_reader->token.kind)
  {
    case TI_JSON_TOKEN_NONE:
    {
      return _ti_json_reader_read_first_token(ref_json_reader, json, first_byte);
    }
    case TI_JSON_TOKEN_BEGIN_OBJECT:
    {
      if (first_byte == '}')
      {
        return _ti_json_reader_process_container_end(ref_json_reader, TI_JSON_TOKEN_END_OBJECT);
      }

      // We expect the start of a property name as the first non-whitespace character within a
      // JSON object.
      if (first_byte != '"')
      {
        return TI_ERROR_UNEXPECTED_CHAR;
      }
      return _ti_json_reader_process_property_name(ref_json_reader);
    }
    case TI_JSON_TOKEN_BEGIN_ARRAY:
    {
      if (first_byte == ']')
      {
        return _ti_json_reader_process_container_end(ref_json_reader, TI_JSON_TOKEN_END_ARRAY);
      }

      return _ti_json_reader_process_value(ref_json_reader, first_byte);
    }
    case TI_JSON_TOKEN_PROPERTY_NAME:
      return _ti_json_reader_process_value(ref_json_reader, first_byte);
    case TI_JSON_TOKEN_END_OBJECT:
    case TI_JSON_TOKEN_END_ARRAY:
    case TI_JSON_TOKEN_STRING:
    case TI_JSON_TOKEN_NUMBER:
    case TI_JSON_TOKEN_TRUE:
    case TI_JSON_TOKEN_FALSE:
    case TI_JSON_TOKEN_NULL:
      return _ti_json_reader_process_next_byte(ref_json_reader, first_byte);
    default:
      return TI_ERROR_JSON_INVALID_STATE;
  }
}

TI_NODISCARD ti_result ti_json_reader_skip_children(ti_json_reader* ref_json_reader)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_reader);

  if (ref_json_reader->token.kind == TI_JSON_TOKEN_PROPERTY_NAME)
  {
    _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));
  }

  ti_json_token_kind const token_kind = ref_json_reader->token.kind;
  if (token_kind == TI_JSON_TOKEN_BEGIN_OBJECT || token_kind == TI_JSON_TOKEN_BEGIN_ARRAY)
  {
    // Keep moving the reader until we come back to the same depth.
    int32_t const depth = ref_json_reader->_internal.bit_stack._internal.current_depth;
    do
    {
      _ti_RETURN_IF_FAILED(ti_json_reader_next_token(ref_json_reader));
    } while (depth <= ref_json_reader->_internal.bit_stack._internal.current_depth);
  }
  return TI_OK;
}
