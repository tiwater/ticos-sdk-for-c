//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Convenience utilities that can be used with coredumps

#include "ticos/panics/coredump.h"

#include "ticos/core/debug_log.h"

bool ticos_coredump_storage_check_size(void) {
  sTcsCoredumpStorageInfo storage_info = { 0 };
  ticos_platform_coredump_storage_get_info(&storage_info);
  const size_t size_needed = ticos_coredump_storage_compute_size_required();
  if (size_needed <= storage_info.size) {
    return true;
  }

  TICOS_LOG_ERROR("Coredump storage is %dB but need %dB",
                     (int)storage_info.size, (int)size_needed);
  return false;
}
