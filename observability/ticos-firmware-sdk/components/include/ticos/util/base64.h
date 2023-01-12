#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Utilities for base64 encoding binary data

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Computes how many bytes will be needed to encode a binary blob of bin_len
//! using base64
#define TICOS_BASE64_ENCODE_LEN(bin_len) (4 * (((bin_len) + 2) / 3))

//! Computes the maximum size a base64 binary blob will wind up being when decoded back into binary
//! (It is possible the actual size is up to 2 bytes less in length if padding bytes were added)
#define TICOS_BASE64_MAX_DECODE_LEN(base64_len) ((3 * (base64_len)) / 4)

//! Base64 encode a given binary buffer
//!
//! @note Uses the standard base64 alphabet from https://tools.ietf.org/html/rfc4648#section-4
//!
//! @param[in] bin Pointer to the binary buffer to base64 encode
//! @param[in] bin_len Length of the binary buffer
//! @param[out] pointer to buffer to write base64 encoded data into. The length of the buffer must
//!    be >= TICOS_BASE64_ENCODE_LEN(bin_len) bytes
void ticos_base64_encode(const void *buf, size_t buf_len, void *base64_out);

//! Base64 encode a given binary buffer in place
//!
//! @note Uses the standard base64 alphabet from https://tools.ietf.org/html/rfc4648#section-4
//!
//! @param[in, out] buf Pointer to the binary buffer to base64 encode. The total length of the
//!    buffer must be >= TICOS_BASE64_ENCODE_LEN(bin_len) bytes since we will be encoding in
//!    place
//! @param[in] bin_len Length of the binary data starting at buf[0] to be base64 encoded.
void ticos_base64_encode_inplace(void *buf, size_t bin_len);

#ifdef __cplusplus
}
#endif
