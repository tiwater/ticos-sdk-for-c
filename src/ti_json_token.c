// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <ti_precondition_internal.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include "ti_json_private.h"

#include "ti_span_private.h"
#include <_ti_cfg.h>

static ti_span _ti_json_token_copy_into_span_helper(
    ti_json_token const* json_token,
    ti_span destination)
{
  _ti_PRECONDITION(json_token->_internal.is_multisegment);

  if (json_token->size == 0)
  {
    return destination;
  }

  for (int32_t i = json_token->_internal.start_buffer_index;
       i <= json_token->_internal.end_buffer_index;
       i++)
  {
    ti_span source = json_token->_internal.pointer_to_first_buffer[i];
    if (i == json_token->_internal.start_buffer_index)
    {
      source = ti_span_slice_to_end(source, json_token->_internal.start_buffer_offset);
    }
    else if (i == json_token->_internal.end_buffer_index)
    {
      source = ti_span_slice(source, 0, json_token->_internal.end_buffer_offset);
    }
    destination = ti_span_copy(destination, source);
  }

  return destination;
}

ti_span ti_json_token_copy_into_span(ti_json_token const* json_token, ti_span destination)
{
  _ti_PRECONDITION_VALID_SPAN(destination, json_token->size, false);

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_copy(destination, token_slice);
  }

  // Token straddles more than one segment
  return _ti_json_token_copy_into_span_helper(json_token, destination);
}

TI_NODISCARD static uint8_t _ti_json_unescape_single_byte(uint8_t ch)
{
  switch (ch)
  {
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case '\\':
    case '"':
    case '/':
    default:
    {
      // We are assuming the JSON token string has already been validated before this and we won't
      // have unexpected bytes folowing the back slash (for example \q). Therefore, just return the
      // same character back for such cases.
      return ch;
    }
  }
}

TI_NODISCARD static bool _ti_json_token_is_text_equal_helper(
    ti_span token_slice,
    ti_span* expected_text,
    bool* next_char_escaped)
{
  int32_t token_size = ti_span_size(token_slice);
  uint8_t* token_ptr = ti_span_ptr(token_slice);

  int32_t expected_size = ti_span_size(*expected_text);
  uint8_t* expected_ptr = ti_span_ptr(*expected_text);

  int32_t token_idx = 0;
  for (int32_t i = 0; i < expected_size; i++)
  {
    if (token_idx >= token_size)
    {
      *expected_text = ti_span_slice_to_end(*expected_text, i);
      return false;
    }
    uint8_t token_byte = token_ptr[token_idx];

    if (token_byte == '\\' || *next_char_escaped)
    {
      if (*next_char_escaped)
      {
        token_byte = _ti_json_unescape_single_byte(token_byte);
      }
      else
      {
        token_idx++;
        if (token_idx >= token_size)
        {
          *next_char_escaped = true;
          *expected_text = ti_span_slice_to_end(*expected_text, i);
          return false;
        }
        token_byte = _ti_json_unescape_single_byte(token_ptr[token_idx]);
      }
      *next_char_escaped = false;

      // TODO: Characters escaped in the form of \uXXXX where XXXX is the UTF-16 code point, is
      // not currently supported.
      // To do this, we need to encode UTF-16 codepoints (including surrogate pairs) into UTF-8.
      if (token_byte == 'u')
      {
        *expected_text = TI_SPAN_EMPTY;
        return false;
      }
    }

    if (token_byte != expected_ptr[i])
    {
      *expected_text = TI_SPAN_EMPTY;
      return false;
    }

    token_idx++;
  }

  *expected_text = TI_SPAN_EMPTY;

  // Only return true if the size of the unescaped token matches the expected size exactly.
  return token_idx == token_size;
}

TI_NODISCARD bool ti_json_token_is_text_equal(
    ti_json_token const* json_token,
    ti_span expected_text)
{
  _ti_PRECONDITION_NOT_NULL(json_token);

  // Cannot compare the value of non-string token kinds
  if (json_token->kind != TI_JSON_TOKEN_STRING && json_token->kind != TI_JSON_TOKEN_PROPERTY_NAME)
  {
    return false;
  }

  ti_span token_slice = json_token->slice;

  // There is nothing to unescape here, compare directly.
  if (!json_token->_internal.string_has_escaped_chars)
  {
    // Contiguous token
    if (!json_token->_internal.is_multisegment)
    {
      return ti_span_is_content_equal(token_slice, expected_text);
    }

    // Token straddles more than one segment
    for (int32_t i = json_token->_internal.start_buffer_index;
         i <= json_token->_internal.end_buffer_index;
         i++)
    {
      ti_span source = json_token->_internal.pointer_to_first_buffer[i];
      if (i == json_token->_internal.start_buffer_index)
      {
        source = ti_span_slice_to_end(source, json_token->_internal.start_buffer_offset);
      }
      else if (i == json_token->_internal.end_buffer_index)
      {
        source = ti_span_slice(source, 0, json_token->_internal.end_buffer_offset);
      }

      int32_t source_size = ti_span_size(source);
      if (ti_span_size(expected_text) < source_size
          || !ti_span_is_content_equal(source, ti_span_slice(expected_text, 0, source_size)))
      {
        return false;
      }
      expected_text = ti_span_slice_to_end(expected_text, source_size);
    }
    // Only return true if we have gone through and compared the entire expected_text.
    return ti_span_size(expected_text) == 0;
  }

  int32_t token_size = json_token->size;
  int32_t expected_size = ti_span_size(expected_text);

  // No need to try to unescape the token slice, since the lengths won't match anyway.
  // Unescaping always shrinks the string, at most by a factor of 6.
  if (token_size < expected_size
      || (token_size / _ti_MAX_EXPANSION_FACTOR_WHILE_ESCAPING) > expected_size)
  {
    return false;
  }

  bool next_char_escaped = false;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return _ti_json_token_is_text_equal_helper(token_slice, &expected_text, &next_char_escaped);
  }

  // Token straddles more than one segment
  for (int32_t i = json_token->_internal.start_buffer_index;
       i <= json_token->_internal.end_buffer_index;
       i++)
  {
    ti_span source = json_token->_internal.pointer_to_first_buffer[i];
    if (i == json_token->_internal.start_buffer_index)
    {
      source = ti_span_slice_to_end(source, json_token->_internal.start_buffer_offset);
    }
    else if (i == json_token->_internal.end_buffer_index)
    {
      source = ti_span_slice(source, 0, json_token->_internal.end_buffer_offset);
    }

    if (!_ti_json_token_is_text_equal_helper(source, &expected_text, &next_char_escaped)
        && ti_span_size(expected_text) == 0)
    {
      return false;
    }
  }

  // Only return true if we have gone through and compared the entire expected_text.
  return ti_span_size(expected_text) == 0;
}

TI_NODISCARD ti_result ti_json_token_get_boolean(ti_json_token const* json_token, bool* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_TRUE && json_token->kind != TI_JSON_TOKEN_FALSE)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  // We assume the ti_json_token is well-formed and self-consistent when returned from the
  // ti_json_reader and that if json_token->kind == TI_JSON_TOKEN_TRUE, then the slice contains the
  // characters "true", otherwise it contains "false". Therefore, there is no need to check the
  // contents again.

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    *out_value = ti_span_size(token_slice) == _ti_STRING_LITERAL_LEN("true");
  }
  else
  {
    // Token straddles more than one segment
    *out_value = json_token->size == _ti_STRING_LITERAL_LEN("true");
  }

  return TI_OK;
}

TI_NODISCARD static ti_result _ti_json_token_get_string_helper(
    ti_span source,
    char* destination,
    int32_t destination_max_size,
    int32_t* dest_idx,
    bool* next_char_escaped)
{
  int32_t source_size = ti_span_size(source);
  uint8_t* source_ptr = ti_span_ptr(source);
  for (int32_t i = 0; i < source_size; i++)
  {
    if (*dest_idx >= destination_max_size)
    {
      return TI_ERROR_NOT_ENOUGH_SPACE;
    }
    uint8_t token_byte = source_ptr[i];

    if (token_byte == '\\' || *next_char_escaped)
    {
      if (*next_char_escaped)
      {
        token_byte = _ti_json_unescape_single_byte(token_byte);
      }
      else
      {
        i++;
        if (i >= source_size)
        {
          *next_char_escaped = true;
          break;
        }
        token_byte = _ti_json_unescape_single_byte(source_ptr[i]);
      }
      *next_char_escaped = false;

      // TODO: Characters escaped in the form of \uXXXX where XXXX is the UTF-16 code point, is
      // not currently supported.
      // To do this, we need to encode UTF-16 codepoints (including surrogate pairs) into UTF-8.
      if (token_byte == 'u')
      {
        return TI_ERROR_NOT_IMPLEMENTED;
      }
    }

    destination[*dest_idx] = (char)token_byte;
    *dest_idx = *dest_idx + 1;
  }

  return TI_OK;
}

TI_NODISCARD ti_result ti_json_token_get_string(
    ti_json_token const* json_token,
    char* destination,
    int32_t destination_max_size,
    int32_t* out_string_length)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(destination);
  _ti_PRECONDITION(destination_max_size > 0);

  if (json_token->kind != TI_JSON_TOKEN_STRING && json_token->kind != TI_JSON_TOKEN_PROPERTY_NAME)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;
  int32_t token_size = json_token->size;

  // There is nothing to unescape here, copy directly.
  if (!json_token->_internal.string_has_escaped_chars)
  {
    // We need enough space to add a null terminator.
    if (token_size >= destination_max_size)
    {
      return TI_ERROR_NOT_ENOUGH_SPACE;
    }

    // Contiguous token
    if (!json_token->_internal.is_multisegment)
    {
      // This will add a null terminator.
      ti_span_to_str(destination, destination_max_size, token_slice);
    }
    else
    {
      // Token straddles more than one segment
      ti_span remainder = _ti_json_token_copy_into_span_helper(
          json_token, ti_span_create((uint8_t*)destination, destination_max_size));

      // Add a null terminator.
      ti_span_copy_u8(remainder, 0);
    }

    if (out_string_length != NULL)
    {
      *out_string_length = token_size;
    }
    return TI_OK;
  }

  // No need to try to unescape the token slice, if the destination is known to be too small.
  // Unescaping always shrinks the string, at most by a factor of 6.
  // We also need enough space to add a null terminator.
  if (token_size / _ti_MAX_EXPANSION_FACTOR_WHILE_ESCAPING >= destination_max_size)
  {
    return TI_ERROR_NOT_ENOUGH_SPACE;
  }

  int32_t dest_idx = 0;
  bool next_char_escaped = false;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    _ti_RETURN_IF_FAILED(_ti_json_token_get_string_helper(
        token_slice, destination, destination_max_size, &dest_idx, &next_char_escaped));
  }
  else
  {
    // Token straddles more than one segment
    for (int32_t i = json_token->_internal.start_buffer_index;
         i <= json_token->_internal.end_buffer_index;
         i++)
    {
      ti_span source = json_token->_internal.pointer_to_first_buffer[i];
      if (i == json_token->_internal.start_buffer_index)
      {
        source = ti_span_slice_to_end(source, json_token->_internal.start_buffer_offset);
      }
      else if (i == json_token->_internal.end_buffer_index)
      {
        source = ti_span_slice(source, 0, json_token->_internal.end_buffer_offset);
      }

      _ti_RETURN_IF_FAILED(_ti_json_token_get_string_helper(
          source, destination, destination_max_size, &dest_idx, &next_char_escaped));
    }
  }

  if (dest_idx >= destination_max_size)
  {
    return TI_ERROR_NOT_ENOUGH_SPACE;
  }
  destination[dest_idx] = 0;

  if (out_string_length != NULL)
  {
    *out_string_length = dest_idx;
  }

  return TI_OK;
}

TI_NODISCARD ti_result
ti_json_token_get_uint64(ti_json_token const* json_token, uint64_t* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_NUMBER)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_atou64(token_slice, out_value);
  }

  // Any number that won't fit in the scratch buffer, will overflow.
  if (json_token->size > _ti_MAX_SIZE_FOR_UINT64)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // Token straddles more than one segment.
  // Used to copy discontiguous token values into a contiguous buffer, for number parsing.
  uint8_t scratch_buffer[_ti_MAX_SIZE_FOR_UINT64] = { 0 };
  ti_span scratch = TI_SPAN_FROM_BUFFER(scratch_buffer);

  ti_span remainder = _ti_json_token_copy_into_span_helper(json_token, scratch);

  return ti_span_atou64(ti_span_slice(scratch, 0, _ti_span_diff(remainder, scratch)), out_value);
}

TI_NODISCARD ti_result
ti_json_token_get_uint32(ti_json_token const* json_token, uint32_t* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_NUMBER)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_atou32(token_slice, out_value);
  }

  // Any number that won't fit in the scratch buffer, will overflow.
  if (json_token->size > _ti_MAX_SIZE_FOR_UINT32)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // Token straddles more than one segment.
  // Used to copy discontiguous token values into a contiguous buffer, for number parsing.
  uint8_t scratch_buffer[_ti_MAX_SIZE_FOR_UINT32] = { 0 };
  ti_span scratch = TI_SPAN_FROM_BUFFER(scratch_buffer);

  ti_span remainder = _ti_json_token_copy_into_span_helper(json_token, scratch);

  return ti_span_atou32(ti_span_slice(scratch, 0, _ti_span_diff(remainder, scratch)), out_value);
}

TI_NODISCARD ti_result ti_json_token_get_int64(ti_json_token const* json_token, int64_t* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_NUMBER)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_atoi64(token_slice, out_value);
  }

  // Any number that won't fit in the scratch buffer, will overflow.
  if (json_token->size > _ti_MAX_SIZE_FOR_INT64)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // Token straddles more than one segment.
  // Used to copy discontiguous token values into a contiguous buffer, for number parsing.
  uint8_t scratch_buffer[_ti_MAX_SIZE_FOR_INT64] = { 0 };
  ti_span scratch = TI_SPAN_FROM_BUFFER(scratch_buffer);

  ti_span remainder = _ti_json_token_copy_into_span_helper(json_token, scratch);

  return ti_span_atoi64(ti_span_slice(scratch, 0, _ti_span_diff(remainder, scratch)), out_value);
}

TI_NODISCARD ti_result ti_json_token_get_int32(ti_json_token const* json_token, int32_t* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_NUMBER)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_atoi32(token_slice, out_value);
  }

  // Any number that won't fit in the scratch buffer, will overflow.
  if (json_token->size > _ti_MAX_SIZE_FOR_INT32)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // Token straddles more than one segment.
  // Used to copy discontiguous token values into a contiguous buffer, for number parsing.
  uint8_t scratch_buffer[_ti_MAX_SIZE_FOR_INT32] = { 0 };
  ti_span scratch = TI_SPAN_FROM_BUFFER(scratch_buffer);

  ti_span remainder = _ti_json_token_copy_into_span_helper(json_token, scratch);

  return ti_span_atoi32(ti_span_slice(scratch, 0, _ti_span_diff(remainder, scratch)), out_value);
}

TI_NODISCARD ti_result ti_json_token_get_double(ti_json_token const* json_token, double* out_value)
{
  _ti_PRECONDITION_NOT_NULL(json_token);
  _ti_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != TI_JSON_TOKEN_NUMBER)
  {
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span token_slice = json_token->slice;

  // Contiguous token
  if (!json_token->_internal.is_multisegment)
  {
    return ti_span_atod(token_slice, out_value);
  }

  // Any number that won't fit in the scratch buffer, will overflow.
  if (json_token->size > _ti_MAX_SIZE_FOR_PARSING_DOUBLE)
  {
    return TI_ERROR_UNEXPECTED_CHAR;
  }

  // Token straddles more than one segment.
  // Used to copy discontiguous token values into a contiguous buffer, for number parsing.
  uint8_t scratch_buffer[_ti_MAX_SIZE_FOR_PARSING_DOUBLE] = { 0 };
  ti_span scratch = TI_SPAN_FROM_BUFFER(scratch_buffer);

  ti_span remainder = _ti_json_token_copy_into_span_helper(json_token, scratch);

  return ti_span_atod(ti_span_slice(scratch, 0, _ti_span_diff(remainder, scratch)), out_value);
}
