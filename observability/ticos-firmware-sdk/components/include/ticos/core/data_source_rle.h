#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief

//! A generic data source implementation that can wrap a pre-existing data source
//! (e.g. g_ticos_coredump_data_source) and compress the stream using Run-length Encoding (RLE).
//!
//! The feature is enabled by default but can be disabled by simply excluding
//! 'ticos_data_source_rle.c' from your compilation list or adding the define
//! TICOS_DATA_SOURCE_RLE_ENABLED=0 as a define to your build system.
//!
//! @note If your setup relies on accessing data sources asynchronously
//! (https://ticos.io/data-to-cloud-async-mode), you will need to disable this feature.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ticos/core/data_packetizer_source.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ticos_data_source_rle_encoder_set_active(const sTicosDataSourceImpl *active_source);
bool ticos_data_source_rle_has_more_msgs(size_t *total_size);
bool ticos_data_source_rle_read_msg(uint32_t offset, void *buf, size_t buf_len);
void ticos_data_source_rle_mark_msg_read(void);

extern const sTicosDataSourceImpl g_ticos_data_rle_source;

#ifdef __cplusplus
}
#endif
