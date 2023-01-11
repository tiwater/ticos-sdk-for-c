#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! A utility that implements a small subset of the CBOR RFC:
//!  https://tools.ietf.org/html/rfc7049
//!
//!
//! CONTEXT: The Ticos metric events API serializes data out to CBOR. Since the actual CBOR
//! serialization feature set needed by the SDK is a tiny subset of the CBOR RFC, a minimal
//! implementation is implemented here.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! The context used to track an active cbor encoding operation
//! A consumer of this API should never have to access the structure directly
typedef struct TicosCborEncoder sTicosCborEncoder;

//! The backing storage to write the encoded data to
//!
//! @param ctx The context provided as part of "ticos_cbor_encoder_init"

//! @param offset The offset within the storage to write to. These offsets are guaranteed to be
//!   sequential. (i.e If the last write wrote 3 bytes at offset 0, the next write_cb will begin at
//!   offset 3) The offset is returned as a convenience. For example if the backing storage is a RAM
//! buffer with no state-tracking of it's own
//! @param buf The payload to write to the storage
//! @param buf_len The size of the payload to write
typedef void (TicosCborWriteCallback)(void *ctx, uint32_t offset, const void *buf, size_t buf_len);

//! Initializes the 'encoder' structure. Must be called at the start of any new encoding
//!
//! @param encoder Context structure initialized by this routine for tracking active encoding state
//! @param write_cb The callback to be invoked when a write needs to be performed
//! @param context The context to be provided along with the write_cb
//! @param buf_len The space free in the backing storage being encoded to. The encoder API
//!  will _never_ attempt to write more bytes than this
void ticos_cbor_encoder_init(sTicosCborEncoder *encoder, TicosCborWriteCallback *cb,
                                void *context, size_t buf_len);

//! Same as "ticos_cbor_encoder_init" but instead of encoding to a buffer will
//! only set the encoder up to compute the total size of the encode
//!
//! When encoding is done and "ticos_cbort_encoder_deinit" is called the total
//! encoding size will be returned
void ticos_cbor_encoder_size_only_init(sTicosCborEncoder *encoder);

//! Resets the state of the encoder context
//!
//! @return the number of bytes successfully encoded
size_t ticos_cbor_encoder_deinit(sTicosCborEncoder *encoder);

//! Called to begin the encoding of a dictionary (also known as a map, object, hashes)
//!
//! @param encoder The encoder context to use
//! @param num_elements The number of pairs of data items that will be in the dictionary
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_dictionary_begin(sTicosCborEncoder *encoder, size_t num_elements);


//! Called to begin the encoding of an array (also referred to as a list, sequence, or tuple)
//!
//! @param encoder The encoder context to use
//! @param num_elements The number of data items that will be in the array
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_array_begin(sTicosCborEncoder *encoder, size_t num_elements);

//! Called to encode an unsigned 32-bit integer data item
//!
//! @param encoder The encoder context to use
//! @param value The value to store
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_unsigned_integer(sTicosCborEncoder *encoder, uint32_t value);

//! Same as "ticos_cbor_encode_unsigned_integer" but store an unsigned integer instead
bool ticos_cbor_encode_signed_integer(sTicosCborEncoder *encoder, int32_t value);

//! Adds pre-encoded cbor data to the current encoder
//!
//! @param encoder The encoder context to use
//! @param cbor_data The pre-encoded data to add to the current context
//! @param cbor_data_len The length of the pre-encoded data
//!
//! @note Care must be taken by the end user to ensure the data being joined into the current
//! encoding creates a valid cbor entry when combined. This utility can helpful, for example, when
//! adding a value to a cbor dictionary/map which is a cbor record itself.
bool ticos_cbor_join(sTicosCborEncoder *encoder, const void *cbor_data, size_t cbor_data_len);

//! Called to encode an arbitrary binary payload
//!
//! @param encoder The encoder context to use
//! @param buf The buffer to store
//! @param buf_len The length of the buffer to store
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_byte_string(sTicosCborEncoder *encoder, const void *buf,
                                      size_t buf_len);

//! Called to encode a NUL terminated C string
//!
//! @param encoder The encoder context to use
//! @param str The string to store
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_string(sTicosCborEncoder *encoder, const char *str);

//! Called to start the encoding of a C string
//!
//! @param encoder The encoder context to use
//! @param str_len The length of the string to store in bytes, excluding NULL terminator.
//!
//! @return true on success, false otherwise
//!
//! @note Use one or more calls to ticos_cbor_join() to write the contents
//! of the string.
bool ticos_cbor_encode_string_begin(sTicosCborEncoder *encoder, size_t str_len);

//! Called to start the encoding of an arbitrary binary payload
//!
//! @param encoder The encoder context to use
//! @param buf_len The length of the binary payload to store in bytes, excluding NULL terminator.
//!
//! @return true on success, false otherwise
//!
//! @note Use one or more calls to ticos_cbor_join() to write the contents
//! of the string.
bool ticos_cbor_encode_byte_string_begin(sTicosCborEncoder *encoder, size_t bin_len);

//! Encodes a IEEE 754 double-precision float that is packed in a uint64_t
//!
//! @param encoder The encoder context to use
//! @param val The value of the float to encode
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_uint64_as_double(sTicosCborEncoder *encoder, uint64_t value);


//! Called to encode a signed 64 bit data item
//!
//! @param encode The encoder context to use
//! @param value The value to store
//!
//! @return true on success, false otherwise
bool ticos_cbor_encode_long_signed_integer(sTicosCborEncoder *encoder, int64_t value);


//! NOTE: For internal use only, included in the header so it's easy for a caller to statically
//! allocate the structure
struct TicosCborEncoder {
  bool compute_size_only;
  TicosCborWriteCallback *write_cb;
  void *write_cb_ctx;
  size_t buf_len;

  size_t encoded_size;
};

#ifdef __cplusplus
}
#endif
