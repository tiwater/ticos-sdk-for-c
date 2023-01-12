#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! APIs intended for internal use _only_. And end user should never invoke these

#ifdef __cplusplus
extern "C" {
#endif

//! Resets Cdr for unit testing purposes.
void ticos_cdr_source_reset(void);

#ifdef __cplusplus
}
#endif
