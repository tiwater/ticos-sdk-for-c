// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "ti_hex_private.h"
#include "ti_json_private.h"
#include "ti_span_private.h"
#include <ti_json.h>
#include <ti_result_internal.h>
#include <ti_span_internal.h>

#include <math.h>

#include <_ti_cfg.h>

TI_NODISCARD ti_result ti_json_writer_init(
    ti_json_writer* out_json_writer,
    ti_span destination_buffer,
    ti_json_writer_options const* options)
{
  _ti_PRECONDITION_NOT_NULL(out_json_writer);

  *out_json_writer = (ti_json_writer){
    .total_bytes_written = 0,
    ._internal = {
      .destination_buffer = destination_buffer,
      .allocator_callback = NULL,
      .user_context = NULL,
      .bytes_written = 0,
      .need_comma = false,
      .token_kind = TI_JSON_TOKEN_NONE,
      .bit_stack = { 0 },
      .options = options == NULL ? ti_json_writer_options_default() : *options,
    },
  };
  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_chunked_init(
    ti_json_writer* out_json_writer,
    ti_span first_destination_buffer,
    ti_span_allocator_fn allocator_callback,
    void* user_context,
    ti_json_writer_options const* options)
{
  _ti_PRECONDITION_NOT_NULL(out_json_writer);
  _ti_PRECONDITION_NOT_NULL(allocator_callback);

  *out_json_writer = (ti_json_writer){
    .total_bytes_written = 0,
    ._internal = {
      .destination_buffer = first_destination_buffer,
      .allocator_callback = allocator_callback,
      .user_context = user_context,
      .bytes_written = 0,
      .need_comma = false,
      .token_kind = TI_JSON_TOKEN_NONE,
      .bit_stack = { 0 },
      .options = options == NULL ? ti_json_writer_options_default() : *options,
    },
  };
  return TI_OK;
}

static TI_NODISCARD ti_span
_get_remaining_span(ti_json_writer* ref_json_writer, int32_t required_size)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(required_size > 0);

  ti_span remaining = ti_span_slice_to_end(
      ref_json_writer->_internal.destination_buffer, ref_json_writer->_internal.bytes_written);

  if (ti_span_size(remaining) < required_size
      && ref_json_writer->_internal.allocator_callback != NULL)
  {
    ti_span_allocator_context context = {
      .user_context = ref_json_writer->_internal.user_context,
      .bytes_used = ref_json_writer->_internal.bytes_written,
      .minimum_required_size = required_size,
    };

    // No more space left in the destination, let the caller fail with TI_ERROR_NOT_ENOUGH_SPACE.
    if (ti_result_failed(ref_json_writer->_internal.allocator_callback(&context, &remaining)))
    {
      return TI_SPAN_EMPTY;
    }
    ref_json_writer->_internal.destination_buffer = remaining;
    ref_json_writer->_internal.bytes_written = 0;
  }

  return remaining;
}

// This validation method is used outside of just preconditions, within
// ti_json_writer_append_json_text.
static TI_NODISCARD bool _ti_is_appending_value_valid(ti_json_writer const* json_writer)
{
  _ti_PRECONDITION_NOT_NULL(json_writer);

  ti_json_token_kind kind = json_writer->_internal.token_kind;

  if (_ti_json_stack_peek(&json_writer->_internal.bit_stack))
  {
    // Cannot write a JSON value within an object without a property name first.
    // That includes writing the start of an object or array without a property name.
    if (kind != TI_JSON_TOKEN_PROPERTY_NAME)
    {
      // Given we are within a JSON object, kind cannot be start of an array or none.
      _ti_PRECONDITION(kind != TI_JSON_TOKEN_NONE && kind != TI_JSON_TOKEN_BEGIN_ARRAY);

      return false;
    }
  }
  else
  {
    // Adding a JSON value within a JSON array is allowed and it is also allowed to add a standalone
    // single JSON value. However, it is invalid to add multiple JSON values that aren't within a
    // container, or outside an existing closed object/array.

    // That includes writing the start of an object or array after a single JSON value or outside of
    // an existing closed object/array.

    // Given we are not within a JSON object, kind cannot be property name.
    _ti_PRECONDITION(kind != TI_JSON_TOKEN_PROPERTY_NAME && kind != TI_JSON_TOKEN_BEGIN_OBJECT);

    // It is more likely for current_depth to not equal 0 when writing valid JSON, so check that
    // first to rely on short-circuiting and return quickly.
    if (json_writer->_internal.bit_stack._internal.current_depth == 0 && kind != TI_JSON_TOKEN_NONE)
    {
      return false;
    }
  }

  // JSON writer state is valid and a primitive value or start of a container can be appended.
  return true;
}

#ifndef TI_NO_PRECONDITION_CHECKING
static TI_NODISCARD bool _ti_is_appending_property_name_valid(ti_json_writer const* json_writer)
{
  _ti_PRECONDITION_NOT_NULL(json_writer);

  ti_json_token_kind kind = json_writer->_internal.token_kind;

  // Cannot write a JSON property within an array or as the first JSON token.
  // Cannot write a JSON property name following another property name. A JSON value is missing.
  if (!_ti_json_stack_peek(&json_writer->_internal.bit_stack)
      || kind == TI_JSON_TOKEN_PROPERTY_NAME)
  {
    _ti_PRECONDITION(kind != TI_JSON_TOKEN_BEGIN_OBJECT);
    return false;
  }

  // JSON writer state is valid and a property name can be appended.
  return true;
}

static TI_NODISCARD bool _ti_is_appending_container_end_valid(
    ti_json_writer const* json_writer,
    uint8_t byte)
{
  _ti_PRECONDITION_NOT_NULL(json_writer);

  ti_json_token_kind kind = json_writer->_internal.token_kind;

  // Cannot write an end of a container without a matching start.
  // This includes writing the end token as the first token in the JSON or right after a property
  // name.
  if (json_writer->_internal.bit_stack._internal.current_depth <= 0
      || kind == TI_JSON_TOKEN_PROPERTY_NAME)
  {
    return false;
  }

  _ti_json_stack_item stack_item = _ti_json_stack_peek(&json_writer->_internal.bit_stack);

  if (byte == ']')
  {
    // If inside a JSON object, then appending an end bracket is invalid:
    if (stack_item)
    {
      _ti_PRECONDITION(kind != TI_JSON_TOKEN_NONE);
      return false;
    }
  }
  else
  {
    _ti_PRECONDITION(byte == '}');

    // If not inside a JSON object, then appending an end brace is invalid:
    if (!stack_item)
    {
      return false;
    }
  }

  // JSON writer state is valid and an end of a container can be appended.
  return true;
}
#endif // TI_NO_PRECONDITION_CHECKING

// Returns the length of the JSON string within the ti_span after it has been escaped.
// The out parameter contains the index where the first character to escape is found.
// If no chars need to be escaped then return the size of value with the out parameter set to -1.
// If break_on_first_escaped is set to true, then it returns as soon as the first character to
// escape is found.
static int32_t _ti_json_writer_escaped_length(
    ti_span value,
    int32_t* out_index_of_first_escaped_char,
    bool break_on_first_escaped)
{
  _ti_PRECONDITION_NOT_NULL(out_index_of_first_escaped_char);
  _ti_PRECONDITION_VALID_SPAN(value, 0, true);

  int32_t value_size = ti_span_size(value);
  _ti_PRECONDITION(value_size <= _ti_MAX_UNESCAPED_STRING_SIZE);

  int32_t escaped_length = 0;
  *out_index_of_first_escaped_char = -1;

  int32_t i = 0;
  uint8_t* value_ptr = ti_span_ptr(value);

  while (i < value_size)
  {
    uint8_t const ch = value_ptr[i];

    switch (ch)
    {
      case '\\':
      case '"':
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
      {
        escaped_length += 2; // Use the two-character sequence escape for these.
        break;
      }
      default:
      {
        // Check if the character has to be escaped as a UNICODE escape sequence.
        if (ch < _ti_ASCII_SPACE_CHARACTER)
        {
          escaped_length += _ti_MAX_EXPANSION_FACTOR_WHILE_ESCAPING;
        }
        else
        {
          escaped_length++; // No escaping required.
        }
        break;
      }
    }

    i++;

    // If this is the first time that we found a character that needs to be escaped,
    // set out_index_of_first_escaped_char to the corresponding index.
    // If escaped_length == i, then we haven't found a character that needs to be escaped yet.
    if (escaped_length != i && *out_index_of_first_escaped_char == -1)
    {
      *out_index_of_first_escaped_char = i - 1;
      if (break_on_first_escaped)
      {
        break;
      }
    }

    // If the length overflows, in case the precondition is not honored, stop processing and break
    // The caller will return TI_ERROR_NOT_ENOUGH_SPACE since ti_span can't contain it.
    // TODO: Consider removing this if it is too costly.
    if (escaped_length < 0)
    {
      escaped_length = INT32_MAX;
      break;
    }
  }

  // In most common cases, escaped_length will equal value_size and out_index_of_first_escaped_char
  // will be -1.
  return escaped_length;
}

static int32_t _ti_json_writer_escape_next_byte_and_copy(
    ti_span* remaining_destination,
    uint8_t next_byte)
{
  uint8_t escaped = 0;
  int32_t written = 0;

  switch (next_byte)
  {
    case '\\':
    case '"':
    {
      escaped = next_byte;
      break;
    }
    case '\b':
    {
      escaped = 'b';
      break;
    }
    case '\f':
    {
      escaped = 'f';
      break;
    }
    case '\n':
    {
      escaped = 'n';
      break;
    }
    case '\r':
    {
      escaped = 'r';
      break;
    }
    case '\t':
    {
      escaped = 't';
      break;
    }
    default:
    {
      // Check if the character has to be escaped as a UNICODE escape sequence.
      if (next_byte < _ti_ASCII_SPACE_CHARACTER)
      {
        // TODO: Consider moving this array outside the loop.
        uint8_t array[_ti_MAX_EXPANSION_FACTOR_WHILE_ESCAPING] = {
          '\\',
          'u',
          '0',
          '0',
          _ti_number_to_upper_hex((uint8_t)(next_byte / _ti_NUMBER_OF_HEX_VALUES)),
          _ti_number_to_upper_hex((uint8_t)(next_byte % _ti_NUMBER_OF_HEX_VALUES)),
        };
        *remaining_destination = ti_span_copy(*remaining_destination, TI_SPAN_FROM_BUFFER(array));
        written += _ti_MAX_EXPANSION_FACTOR_WHILE_ESCAPING;
      }
      else
      {
        *remaining_destination = ti_span_copy_u8(*remaining_destination, next_byte);
        written++;
      }
      break;
    }
  }

  // If escaped is non-zero, then we found one of the characters that needs to be escaped.
  // Otherwise, we hit the default case in the switch above, in which case, we already wrote
  // the character.
  if (escaped)
  {
    *remaining_destination = ti_span_copy_u8(*remaining_destination, '\\');
    *remaining_destination = ti_span_copy_u8(*remaining_destination, escaped);
    written += 2;
  }
  return written;
}

static TI_NODISCARD ti_span _ti_json_writer_escape_and_copy(ti_span destination, ti_span source)
{
  _ti_PRECONDITION_VALID_SPAN(source, 1, false);

  int32_t src_size = ti_span_size(source);
  _ti_PRECONDITION(src_size <= _ti_MAX_UNESCAPED_STRING_SIZE);
  _ti_PRECONDITION_VALID_SPAN(destination, src_size + 1, false);

  int32_t i = 0;
  uint8_t* value_ptr = ti_span_ptr(source);

  ti_span remaining_destination = destination;

  while (i < src_size)
  {
    uint8_t const ch = value_ptr[i];
    _ti_json_writer_escape_next_byte_and_copy(&remaining_destination, ch);
    i++;
  }

  return remaining_destination;
}

TI_INLINE void _ti_update_json_writer_state(
    ti_json_writer* ref_json_writer,
    int32_t bytes_written_in_last,
    int32_t total_bytes_written,
    bool need_comma,
    ti_json_token_kind token_kind)
{
  ref_json_writer->_internal.bytes_written += bytes_written_in_last;
  ref_json_writer->total_bytes_written += total_bytes_written;
  ref_json_writer->_internal.need_comma = need_comma;
  ref_json_writer->_internal.token_kind = token_kind;
}

static TI_NODISCARD ti_result ti_json_writer_span_copy_chunked(
    ti_json_writer* ref_json_writer,
    ti_span* remaining_json,
    ti_span value)
{
  if (ti_span_size(value) < ti_span_size(*remaining_json))
  {
    *remaining_json = ti_span_copy(*remaining_json, value);
    ref_json_writer->_internal.bytes_written += ti_span_size(value);
  }
  else
  {
    while (ti_span_size(value) != 0)
    {
      int32_t destination_size = ti_span_size(*remaining_json);
      ti_span value_slice_that_fits = value;
      if (destination_size < ti_span_size(value))
      {
        value_slice_that_fits = ti_span_slice(value, 0, destination_size);
      }

      ti_span_copy(*remaining_json, value_slice_that_fits);
      ref_json_writer->_internal.bytes_written += ti_span_size(value_slice_that_fits);

      value = ti_span_slice_to_end(value, ti_span_size(value_slice_that_fits));
      *remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
      _ti_RETURN_IF_NOT_ENOUGH_SIZE(*remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);
    }
  }
  return TI_OK;
}

static TI_NODISCARD ti_result
ti_json_writer_append_string_small(ti_json_writer* ref_json_writer, ti_span value)
{
  _ti_PRECONDITION(ti_span_size(value) <= _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK);

  int32_t required_size = 2; // For the surrounding quotes.

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _ti_json_writer_escaped_length(value, &index_of_first_escaped_char, false);

  _ti_PRECONDITION(required_size <= _ti_MINIMUM_STRING_CHUNK_SIZE);

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  remaining_json = ti_span_copy_u8(remaining_json, '"');

  // No character needed to be escaped, copy the whole string as is.
  if (index_of_first_escaped_char == -1)
  {
    remaining_json = ti_span_copy(remaining_json, value);
  }
  else
  {
    // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
    // encode and copy.
    remaining_json
        = ti_span_copy(remaining_json, ti_span_slice(value, 0, index_of_first_escaped_char));
    remaining_json = _ti_json_writer_escape_and_copy(
        remaining_json, ti_span_slice_to_end(value, index_of_first_escaped_char));
  }

  ti_span_copy_u8(remaining_json, '"');

  _ti_update_json_writer_state(
      ref_json_writer, required_size, required_size, true, TI_JSON_TOKEN_STRING);
  return TI_OK;
}

static TI_NODISCARD ti_result
ti_json_writer_append_string_chunked(ti_json_writer* ref_json_writer, ti_span value)
{
  _ti_PRECONDITION(ti_span_size(value) > _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK);

  ti_span remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

  int32_t required_size = 2; // For the surrounding quotes.
  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
    required_size++;
    ref_json_writer->_internal.bytes_written++;
  }

  remaining_json = ti_span_copy_u8(remaining_json, '"');
  ref_json_writer->_internal.bytes_written++;

  int32_t consumed = 0;
  do
  {
    ti_span value_slice = ti_span_slice_to_end(value, consumed);
    int32_t index_of_first_escaped_char = -1;
    _ti_json_writer_escaped_length(value_slice, &index_of_first_escaped_char, true);

    // No character needed to be escaped, copy the whole string as is.
    if (index_of_first_escaped_char == -1)
    {
      _ti_RETURN_IF_FAILED(
          ti_json_writer_span_copy_chunked(ref_json_writer, &remaining_json, value_slice));
      consumed += ti_span_size(value_slice);
    }
    else
    {
      // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
      // encode and copy.
      _ti_RETURN_IF_FAILED(ti_json_writer_span_copy_chunked(
          ref_json_writer,
          &remaining_json,
          ti_span_slice(value_slice, 0, index_of_first_escaped_char)));

      consumed += index_of_first_escaped_char;

      uint8_t* value_ptr = ti_span_ptr(value_slice);
      uint8_t const ch = value_ptr[index_of_first_escaped_char];

      remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
      _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

      int32_t written = _ti_json_writer_escape_next_byte_and_copy(&remaining_json, ch);
      ref_json_writer->_internal.bytes_written += written;

      // Only account for the difference in the number of bytes written when escaped
      // compared to when the bytes are copied as is.
      required_size += written - 1;

      consumed++;
    }
  } while (consumed < ti_span_size(value));

  remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

  ti_span_copy_u8(remaining_json, '"');
  ref_json_writer->_internal.bytes_written++;

  // Currently, required_size only counts the escaped bytes, so add back the length of the input
  // string (consumed == ti_span_size(value));
  required_size += consumed;

  // We already tracked and updated bytes_written while writing, so no need to update it here.
  _ti_update_json_writer_state(ref_json_writer, 0, required_size, true, TI_JSON_TOKEN_STRING);
  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_append_string(ti_json_writer* ref_json_writer, ti_span value)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  // An empty span is allowed, and we write an empty JSON string for it.
  _ti_PRECONDITION_VALID_SPAN(value, 0, true);
  _ti_PRECONDITION(ti_span_size(value) <= _ti_MAX_UNESCAPED_STRING_SIZE);
  _ti_PRECONDITION(_ti_is_appending_value_valid(ref_json_writer));

  if (ti_span_size(value) <= _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK)
  {
    return ti_json_writer_append_string_small(ref_json_writer, value);
  }

  return ti_json_writer_append_string_chunked(ref_json_writer, value);
}

static TI_NODISCARD ti_result
ti_json_writer_append_property_name_small(ti_json_writer* ref_json_writer, ti_span value)
{
  _ti_PRECONDITION(ti_span_size(value) <= _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK);

  int32_t required_size = 3; // For the surrounding quotes and the key:value separator colon.

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  int32_t index_of_first_escaped_char = -1;
  required_size += _ti_json_writer_escaped_length(value, &index_of_first_escaped_char, false);

  _ti_PRECONDITION(required_size <= _ti_MINIMUM_STRING_CHUNK_SIZE);

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  remaining_json = ti_span_copy_u8(remaining_json, '"');

  // No character needed to be escaped, copy the whole string as is.
  if (index_of_first_escaped_char == -1)
  {
    remaining_json = ti_span_copy(remaining_json, value);
  }
  else
  {
    // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
    // encode and copy.
    remaining_json
        = ti_span_copy(remaining_json, ti_span_slice(value, 0, index_of_first_escaped_char));
    remaining_json = _ti_json_writer_escape_and_copy(
        remaining_json, ti_span_slice_to_end(value, index_of_first_escaped_char));
  }

  remaining_json = ti_span_copy_u8(remaining_json, '"');
  ti_span_copy_u8(remaining_json, ':');

  _ti_update_json_writer_state(
      ref_json_writer, required_size, required_size, false, TI_JSON_TOKEN_PROPERTY_NAME);
  return TI_OK;
}

static TI_NODISCARD ti_result
ti_json_writer_append_property_name_chunked(ti_json_writer* ref_json_writer, ti_span value)
{
  _ti_PRECONDITION(ti_span_size(value) > _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK);

  ti_span remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

  int32_t required_size = 3; // For the surrounding quotes and the key:value separator colon.
  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
    required_size++;
    ref_json_writer->_internal.bytes_written++;
  }

  remaining_json = ti_span_copy_u8(remaining_json, '"');
  ref_json_writer->_internal.bytes_written++;

  int32_t consumed = 0;
  do
  {
    ti_span value_slice = ti_span_slice_to_end(value, consumed);
    int32_t index_of_first_escaped_char = -1;
    _ti_json_writer_escaped_length(value_slice, &index_of_first_escaped_char, true);

    // No character needed to be escaped, copy the whole string as is.
    if (index_of_first_escaped_char == -1)
    {
      _ti_RETURN_IF_FAILED(
          ti_json_writer_span_copy_chunked(ref_json_writer, &remaining_json, value_slice));
      consumed += ti_span_size(value_slice);
    }
    else
    {
      // Bulk copy the characters that didn't need to be escaped before dropping to the byte-by-byte
      // encode and copy.
      _ti_RETURN_IF_FAILED(ti_json_writer_span_copy_chunked(
          ref_json_writer,
          &remaining_json,
          ti_span_slice(value_slice, 0, index_of_first_escaped_char)));

      consumed += index_of_first_escaped_char;

      uint8_t* value_ptr = ti_span_ptr(value_slice);
      uint8_t const ch = value_ptr[index_of_first_escaped_char];

      remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
      _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

      int32_t written = _ti_json_writer_escape_next_byte_and_copy(&remaining_json, ch);
      ref_json_writer->_internal.bytes_written += written;

      // Only account for the difference in the number of bytes written when escaped
      // compared to when the bytes are copied as is.
      required_size += written - 1;

      consumed++;
    }
  } while (consumed < ti_span_size(value));

  remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

  remaining_json = ti_span_copy_u8(remaining_json, '"');
  remaining_json = ti_span_copy_u8(remaining_json, ':');
  ref_json_writer->_internal.bytes_written += 2;

  // Currently, required_size only counts the escaped bytes, so add back the length of the input
  // string (consumed == ti_span_size(value));
  required_size += consumed;

  // We already tracked and updated bytes_written while writing, so no need to update it here.
  _ti_update_json_writer_state(
      ref_json_writer, 0, required_size, false, TI_JSON_TOKEN_PROPERTY_NAME);
  return TI_OK;
}

TI_NODISCARD ti_result
ti_json_writer_append_property_name(ti_json_writer* ref_json_writer, ti_span name)
{
  // TODO: Consider refactoring to reduce duplication between writing property name and string.
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION_VALID_SPAN(name, 0, false);
  _ti_PRECONDITION(ti_span_size(name) <= _ti_MAX_UNESCAPED_STRING_SIZE);
  _ti_PRECONDITION(_ti_is_appending_property_name_valid(ref_json_writer));

  if (ti_span_size(name) <= _ti_MAX_UNESCAPED_STRING_SIZE_PER_CHUNK)
  {
    return ti_json_writer_append_property_name_small(ref_json_writer, name);
  }

  return ti_json_writer_append_property_name_chunked(ref_json_writer, name);
}

static TI_NODISCARD ti_result _ti_validate_json(
    ti_span json_text,
    ti_json_token_kind* first_token_kind,
    ti_json_token_kind* last_token_kind)
{
  _ti_PRECONDITION_NOT_NULL(first_token_kind);

  ti_json_reader reader = { 0 };
  _ti_RETURN_IF_FAILED(ti_json_reader_init(&reader, json_text, NULL));

  ti_result result = ti_json_reader_next_token(&reader);
  _ti_RETURN_IF_FAILED(result);

  // This is guaranteed not to be a property name or end object/array.
  // The first token of a valid JSON must either be a value or start object/array.
  *first_token_kind = reader.token.kind;

  // Keep reading until we have finished validating the entire JSON text and make sure it isn't
  // incomplete.
  while (ti_result_succeeded(result = ti_json_reader_next_token(&reader)))
  {
  }

  if (result != TI_ERROR_JSON_READER_DONE)
  {
    return result;
  }

  // This is guaranteed not to be a property name or start object/array.
  // The last token of a valid JSON must either be a value or end object/array.
  *last_token_kind = reader.token.kind;

  return TI_OK;
}

TI_NODISCARD ti_result
ti_json_writer_append_json_text(ti_json_writer* ref_json_writer, ti_span json_text)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  // A null or empty span is not allowed since that is invalid JSON.
  _ti_PRECONDITION_VALID_SPAN(json_text, 0, false);

  ti_json_token_kind first_token_kind = TI_JSON_TOKEN_NONE;
  ti_json_token_kind last_token_kind = TI_JSON_TOKEN_NONE;

  // This runtime validation is necessary since the input could be user defined and malformed.
  // This cannot be caught at dev time by a precondition, especially since they can be turned off.
  _ti_RETURN_IF_FAILED(_ti_validate_json(json_text, &first_token_kind, &last_token_kind));

  // It is guaranteed that first_token_kind is NOT:
  // TI_JSON_TOKEN_NONE, TI_JSON_TOKEN_END_ARRAY, TI_JSON_TOKEN_END_OBJECT,
  // TI_JSON_TOKEN_PROPERTY_NAME
  // And that last_token_kind is NOT:
  // TI_JSON_TOKEN_NONE, TI_JSON_TOKEN_START_ARRAY, TI_JSON_TOKEN_START_OBJECT,
  // TI_JSON_TOKEN_PROPERTY_NAME

  // The JSON text is valid, but appending it to the the JSON writer at the current state still may
  // not be valid.
  if (!_ti_is_appending_value_valid(ref_json_writer))
  {
    // All other tokens, including start array and object are validated here.
    // Also first_token_kind cannot be TI_JSON_TOKEN_NONE at this point.
    return TI_ERROR_JSON_INVALID_STATE;
  }

  ti_span remaining_json = _get_remaining_span(ref_json_writer, _ti_MINIMUM_STRING_CHUNK_SIZE);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, _ti_MINIMUM_STRING_CHUNK_SIZE);

  int32_t required_size = ti_span_size(json_text);
  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
    ref_json_writer->_internal.bytes_written++;
    required_size++; // For the leading comma separator.
  }

  _ti_RETURN_IF_FAILED(
      ti_json_writer_span_copy_chunked(ref_json_writer, &remaining_json, json_text));

  // We only need to add a comma if the last token we append is a value or end of object/array.
  // If the last token is a property name or the start of an object/array, we don't need to add a
  // comma before appending subsequent tokens.
  // However, there is no valid, complete, single JSON value where the last token would be property
  // name, or start object/array.
  // Therefore, need_comma must be true after appending the json_text.

  // We already tracked and updated bytes_written while writing, so no need to update it here.
  _ti_update_json_writer_state(ref_json_writer, 0, required_size, true, last_token_kind);
  return TI_OK;
}

static TI_NODISCARD ti_result _ti_json_writer_append_literal(
    ti_json_writer* ref_json_writer,
    ti_span literal,
    ti_json_token_kind literal_kind)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(
      literal_kind == TI_JSON_TOKEN_NULL || literal_kind == TI_JSON_TOKEN_TRUE
      || literal_kind == TI_JSON_TOKEN_FALSE);
  _ti_PRECONDITION_VALID_SPAN(literal, 4, false);
  _ti_PRECONDITION(ti_span_size(literal) <= 5); // null, true, or false
  _ti_PRECONDITION(_ti_is_appending_value_valid(ref_json_writer));

  int32_t required_size = ti_span_size(literal);

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  ti_span_copy(remaining_json, literal);

  _ti_update_json_writer_state(ref_json_writer, required_size, required_size, true, literal_kind);
  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_append_bool(ti_json_writer* ref_json_writer, bool value)
{
  return value ? _ti_json_writer_append_literal(
             ref_json_writer, TI_SPAN_FROM_STR("true"), TI_JSON_TOKEN_TRUE)
               : _ti_json_writer_append_literal(
                   ref_json_writer, TI_SPAN_FROM_STR("false"), TI_JSON_TOKEN_FALSE);
}

TI_NODISCARD ti_result ti_json_writer_append_null(ti_json_writer* ref_json_writer)
{
  return _ti_json_writer_append_literal(
      ref_json_writer, TI_SPAN_FROM_STR("null"), TI_JSON_TOKEN_NULL);
}

TI_NODISCARD ti_result ti_json_writer_append_int32(ti_json_writer* ref_json_writer, int32_t value)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(_ti_is_appending_value_valid(ref_json_writer));

  int32_t required_size = _ti_MAX_SIZE_FOR_INT32; // Need enough space to write any 32-bit integer.

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  // Since we asked for the maximum needed space above, this is guaranteed not to fail due to
  // TI_ERROR_NOT_ENOUGH_SPACE. Still checking the returned ti_result, for other potential failure
  // cases.
  ti_span leftover;
  _ti_RETURN_IF_FAILED(ti_span_i32toa(remaining_json, value, &leftover));

  // We already accounted for the maximum size needed in required_size, so subtract that to get the
  // actual bytes written.
  int32_t written
      = required_size + _ti_span_diff(leftover, remaining_json) - _ti_MAX_SIZE_FOR_INT32;
  _ti_update_json_writer_state(ref_json_writer, written, written, true, TI_JSON_TOKEN_NUMBER);
  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_append_double(
    ti_json_writer* ref_json_writer,
    double value,
    int32_t fractional_digits)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(_ti_is_appending_value_valid(ref_json_writer));
  // Non-finite numbers are not supported because they lead to invalid JSON.
  // Unquoted strings such as nan and -inf are invalid as JSON numbers.
  _ti_PRECONDITION(_ti_isfinite(value));
  _ti_PRECONDITION_RANGE(0, fractional_digits, _ti_MAX_SUPPORTED_FRACTIONAL_DIGITS);

  // Need enough space to write any double number.
  int32_t required_size = _ti_MAX_SIZE_FOR_WRITING_DOUBLE;

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  // Since we asked for the maximum needed space above, this is guaranteed not to fail due to
  // TI_ERROR_NOT_ENOUGH_SPACE. Still checking the returned ti_result, for other potential failure
  // cases.
  ti_span leftover;
  _ti_RETURN_IF_FAILED(ti_span_dtoa(remaining_json, value, fractional_digits, &leftover));

  // We already accounted for the maximum size needed in required_size, so subtract that to get the
  // actual bytes written.
  int32_t written
      = required_size + _ti_span_diff(leftover, remaining_json) - _ti_MAX_SIZE_FOR_WRITING_DOUBLE;
  _ti_update_json_writer_state(ref_json_writer, written, written, true, TI_JSON_TOKEN_NUMBER);
  return TI_OK;
}

static TI_NODISCARD ti_result _ti_json_writer_append_container_start(
    ti_json_writer* ref_json_writer,
    uint8_t byte,
    ti_json_token_kind container_kind)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(
      container_kind == TI_JSON_TOKEN_BEGIN_OBJECT || container_kind == TI_JSON_TOKEN_BEGIN_ARRAY);
  _ti_PRECONDITION(_ti_is_appending_value_valid(ref_json_writer));

  // The current depth is equal to or larger than the maximum allowed depth of 64. Cannot write the
  // next JSON object or array.
  if (ref_json_writer->_internal.bit_stack._internal.current_depth >= _ti_MAX_JSON_STACK_SIZE)
  {
    return TI_ERROR_JSON_NESTING_OVERFLOW;
  }

  int32_t required_size = 1; // For the start object or array byte.

  if (ref_json_writer->_internal.need_comma)
  {
    required_size++; // For the leading comma separator.
  }

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  if (ref_json_writer->_internal.need_comma)
  {
    remaining_json = ti_span_copy_u8(remaining_json, ',');
  }

  ti_span_copy_u8(remaining_json, byte);

  _ti_update_json_writer_state(
      ref_json_writer, required_size, required_size, false, container_kind);
  if (container_kind == TI_JSON_TOKEN_BEGIN_OBJECT)
  {
    _ti_json_stack_push(&ref_json_writer->_internal.bit_stack, _ti_JSON_STACK_OBJECT);
  }
  else
  {
    _ti_json_stack_push(&ref_json_writer->_internal.bit_stack, _ti_JSON_STACK_ARRAY);
  }

  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_append_begin_object(ti_json_writer* ref_json_writer)
{
  return _ti_json_writer_append_container_start(ref_json_writer, '{', TI_JSON_TOKEN_BEGIN_OBJECT);
}

TI_NODISCARD ti_result ti_json_writer_append_begin_array(ti_json_writer* ref_json_writer)
{
  return _ti_json_writer_append_container_start(ref_json_writer, '[', TI_JSON_TOKEN_BEGIN_ARRAY);
}

static TI_NODISCARD ti_result ti_json_writer_append_container_end(
    ti_json_writer* ref_json_writer,
    uint8_t byte,
    ti_json_token_kind container_kind)
{
  _ti_PRECONDITION_NOT_NULL(ref_json_writer);
  _ti_PRECONDITION(
      container_kind == TI_JSON_TOKEN_END_OBJECT || container_kind == TI_JSON_TOKEN_END_ARRAY);
  _ti_PRECONDITION(_ti_is_appending_container_end_valid(ref_json_writer, byte));

  int32_t required_size = 1; // For the end object or array byte.

  ti_span remaining_json = _get_remaining_span(ref_json_writer, required_size);
  _ti_RETURN_IF_NOT_ENOUGH_SIZE(remaining_json, required_size);

  ti_span_copy_u8(remaining_json, byte);

  _ti_update_json_writer_state(ref_json_writer, required_size, required_size, true, container_kind);
  _ti_json_stack_pop(&ref_json_writer->_internal.bit_stack);

  return TI_OK;
}

TI_NODISCARD ti_result ti_json_writer_append_end_object(ti_json_writer* ref_json_writer)
{
  return ti_json_writer_append_container_end(ref_json_writer, '}', TI_JSON_TOKEN_END_OBJECT);
}

TI_NODISCARD ti_result ti_json_writer_append_end_array(ti_json_writer* ref_json_writer)
{
  return ti_json_writer_append_container_end(ref_json_writer, ']', TI_JSON_TOKEN_END_ARRAY);
}
