//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A minimal implementation of a CBOR encoder. See header for more details

#include "ticos/util/cbor.h"

#include <inttypes.h>
#include <string.h>

// https://tools.ietf.org/html/rfc7049#section-2.1
typedef enum CborMajorType {
  kCborMajorType_UnsignedInteger = 0,
  kCborMajorType_NegativeInteger = 1,
  kCborMajorType_ByteString = 2,
  kCborMajorType_TextString = 3,
  kCborMajorType_Array = 4,
  kCborMajorType_Map = 5,
  kCborMajorType_Tag = 6,
  kCborMajorType_SimpleType = 7,
} eCborMajorType;

// A CBOR payload is composed of a stream of "data items"
// The main type of each "data item" is known as the CBOR Major Type
// and populates the upper 3 bits of the first byte of each "data item"
#define CBOR_SERIALIZE_MAJOR_TYPE(mt) ((uint8_t) ((mt & 0x7) << 5))

void ticos_cbor_encoder_init(sTicosCborEncoder *encoder, TicosCborWriteCallback write_cb,
                                void *write_cb_ctx, size_t buf_len) {
  const bool compute_size_only = (write_cb == NULL);
  *encoder = (sTicosCborEncoder) {
    .compute_size_only = compute_size_only,
    .write_cb = write_cb,
    .write_cb_ctx = write_cb_ctx,
    .buf_len = buf_len,
  };
}

void ticos_cbor_encoder_size_only_init(sTicosCborEncoder *encoder) {
  ticos_cbor_encoder_init(encoder, NULL, NULL, 0);
}

size_t ticos_cbor_encoder_deinit(sTicosCborEncoder *encoder) {
  const size_t bytes_encoded = encoder->encoded_size;
  *encoder = (sTicosCborEncoder) { 0 };
  return bytes_encoded;
}

static bool prv_add_to_result_buffer(sTicosCborEncoder *encoder, const void *data,
                                     size_t data_len) {
  if (data_len == 0) {
    // no work to do
    return true;
  }

  if (encoder->compute_size_only) {
    // no need to perform size checks because we are only computing the total encoder size
    encoder->encoded_size += data_len;
    return true;
  }

  if ((encoder->encoded_size + data_len) > encoder->buf_len) {
    // not enough space
    return false;
  }
  encoder->write_cb(encoder->write_cb_ctx, encoder->encoded_size, data, data_len);
  encoder->encoded_size += data_len;
  return true;
}

static bool prv_encode_unsigned_integer(
    sTicosCborEncoder *encoder, uint8_t major_type, uint32_t val) {
  uint8_t mt = CBOR_SERIALIZE_MAJOR_TYPE(major_type);

  uint8_t tmp_buf[5];
  uint8_t *p = &tmp_buf[0];

  if (val < 24) {
    *p++ = mt + (uint8_t)val;
  } else {
    if (val <= UINT8_MAX) {
      *p++ = mt + 24;
      *p++ = val & 0xff;
    } else if (val <= UINT16_MAX) {
      *p++ = mt + 25;
      *p++ = (val >> 8) & 0xff;
      *p++ = val & 0xff;
    } else {
      *p++ = mt + 26;
      *p++ = (val >> 24) & 0xff;
      *p++ = (val >> 16) & 0xff;
      *p++ = (val >> 8) & 0xff;
      *p++ = val & 0xff;
    }
  }
  const size_t tmp_buf_len = (uint32_t)(p - tmp_buf);
  return prv_add_to_result_buffer(encoder, tmp_buf, tmp_buf_len);
}

static void prv_encode_uint64(uint8_t buf[8], uint64_t val) {
  uint8_t *p = &buf[0];
  for (int shift = 56; shift >= 0; shift -= 8) {
    *p++ = (val >> shift) & 0xff;
  }
}

#define TICOS_CBOR_UINT64_MAX_ITEM_SIZE_BYTES 9

bool ticos_cbor_encode_long_signed_integer(
    sTicosCborEncoder *encoder, int64_t value) {
  // Logic derived from "Appendix C Pseudocode" of RFC 7049
  int64_t ui = (value >> 63);
  // Figure out if we are encoding a Negative or Unsigned Integer by reading the sign extension bit
  const uint8_t cbor_major_type = ui & 0x1;
  ui ^= value;

  if (ui <= UINT32_MAX) {
    return prv_encode_unsigned_integer(encoder, cbor_major_type, (uint32_t)ui);
  }

  uint8_t tmp_buf[TICOS_CBOR_UINT64_MAX_ITEM_SIZE_BYTES];
  const uint8_t uint64_type_value = 27;
  tmp_buf[0] = CBOR_SERIALIZE_MAJOR_TYPE(cbor_major_type) | uint64_type_value;
  prv_encode_uint64(&tmp_buf[1], (uint64_t)ui);
  return prv_add_to_result_buffer(encoder, tmp_buf, sizeof(tmp_buf));
}

bool ticos_cbor_encode_uint64_as_double(
    sTicosCborEncoder *encoder, uint64_t val) {
  uint8_t tmp_buf[TICOS_CBOR_UINT64_MAX_ITEM_SIZE_BYTES];
  const uint8_t ieee_754_double_precision_float_type = 27;
  tmp_buf[0] = CBOR_SERIALIZE_MAJOR_TYPE(kCborMajorType_SimpleType) |
      ieee_754_double_precision_float_type;
  prv_encode_uint64(&tmp_buf[1], val);
  return prv_add_to_result_buffer(encoder, tmp_buf, sizeof(tmp_buf));
}

bool ticos_cbor_encode_unsigned_integer(
    sTicosCborEncoder *encoder, uint32_t value) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_UnsignedInteger, value);
}

bool ticos_cbor_join(sTicosCborEncoder *encoder, const void *cbor_data, size_t cbor_data_len) {
  return prv_add_to_result_buffer(encoder, cbor_data, cbor_data_len);
}

bool ticos_cbor_encode_signed_integer(sTicosCborEncoder *encoder, int32_t value) {
  // Logic derived from "Appendix C Pseudocode" of RFC 7049
  int32_t ui = (value >> 31);
  // Figure out if we are encoding a Negative or Unsigned Integer by reading a sign extension bit
  const uint8_t cbor_major_type = ui & 0x1;
  ui ^= value;
  return prv_encode_unsigned_integer(encoder, cbor_major_type, (uint32_t)ui);
}

bool ticos_cbor_encode_byte_string(sTicosCborEncoder *encoder, const void *buf,
                                     size_t buf_len) {
  return (prv_encode_unsigned_integer(encoder, kCborMajorType_ByteString, buf_len) &&
          prv_add_to_result_buffer(encoder, buf, buf_len));
}

bool ticos_cbor_encode_byte_string_begin(sTicosCborEncoder *encoder, size_t buf_len) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_ByteString, buf_len);
}

bool ticos_cbor_encode_string(sTicosCborEncoder *encoder, const char *str) {
  const size_t str_len = strlen(str);
  return (prv_encode_unsigned_integer(encoder, kCborMajorType_TextString,  str_len) &&
          prv_add_to_result_buffer(encoder, str, str_len));
}

bool ticos_cbor_encode_string_begin(sTicosCborEncoder *encoder, size_t str_len) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_TextString,  str_len);
}

bool ticos_cbor_encode_dictionary_begin(
    sTicosCborEncoder *encoder, size_t num_elements) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_Map, num_elements);
}

bool ticos_cbor_encode_array_begin(
    sTicosCborEncoder *encoder, size_t num_elements) {
  return prv_encode_unsigned_integer(encoder, kCborMajorType_Array, num_elements);
}
