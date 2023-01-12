#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Math helpers

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define TICOS_MIN(a, b)   (((a) < (b)) ? (a) : (b))
#define TICOS_MAX(a , b)  (((a) > (b)) ? (a) : (b))
#define TICOS_FLOOR(a, align) (((a) / (align)) * (align))
#define TICOS_ABS(a) (((a) < 0) ? -(a) : (a))

#ifdef __cplusplus
}
#endif
