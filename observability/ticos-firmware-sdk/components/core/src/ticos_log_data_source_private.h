#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal logging data source

#include <stdbool.h>

#include "ticos/util/cbor.h"

#ifdef __cplusplus
extern "C" {
#endif

//! @note Internal function
bool ticos_log_data_source_has_been_triggered(void);

//! Reset the state of the logging data source
//!
//! @note Internal function only intended for use with unit tests
void ticos_log_data_source_reset(void);

//! @note Internal function only intended for use with unit tests
size_t ticos_log_data_source_count_unsent_logs(void);

#ifdef __cplusplus
}
#endif
