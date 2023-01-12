//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A stub implementation of ticos_log_save() for unit testing

#include "ticos/core/log.h"

#include "ticos/core/compiler.h"

void ticos_log_save(TICOS_UNUSED eTicosPlatformLogLevel level,
                       TICOS_UNUSED const char *fmt, ...) { }

#if TICOS_COMPACT_LOG_ENABLE
void ticos_compact_log_save(TICOS_UNUSED eTicosPlatformLogLevel level,
                               TICOS_UNUSED uint32_t log_id,
                               TICOS_UNUSED uint32_t compressed_fmt, ...) { }
#endif
