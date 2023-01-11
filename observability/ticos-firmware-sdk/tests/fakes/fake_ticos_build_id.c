//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Fake ticos_build_info_read implementation

#include "fake_ticos_build_id.h"

#include <string.h>

#define INITIAL_FAKE_BUILD_ID (sTicosBuildInfo) { \
  .build_id = { \
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, \
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, \
    0x01, 0x23, 0x45, 0x67, \
  }, \
}

eTicosBuildIdType g_fake_ticos_build_id_type = kTicosBuildIdType_None;
sTicosBuildInfo g_fake_ticos_build_id_info = INITIAL_FAKE_BUILD_ID;

void fake_ticos_build_id_reset(void) {
  g_fake_ticos_build_id_type = kTicosBuildIdType_None;
  g_fake_ticos_build_id_info = INITIAL_FAKE_BUILD_ID;
}

bool ticos_build_info_read(sTicosBuildInfo *info) {
  if (g_fake_ticos_build_id_type == kTicosBuildIdType_None) {
    return false;
  }
  memcpy(info->build_id, g_fake_ticos_build_id_info.build_id,
         sizeof(g_fake_ticos_build_id_info.build_id));
  return true;
}
