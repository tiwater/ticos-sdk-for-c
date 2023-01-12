//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! See header for more details

#include "ticos/core/build_info.h"
#include "ticos_build_id_private.h"

#include <stdint.h>
#include <string.h>

#include "ticos/config.h"

#if TICOS_USE_GNU_BUILD_ID

// Note: This variable is emitted by the linker script
extern uint8_t __start_gnu_build_id_start[];

TICOS_BUILD_ID_QUALIFIER sTicosBuildIdStorage g_ticos_build_id = {
  .type = kTicosBuildIdType_GnuBuildIdSha1,
  .len = sizeof(sTicosElfNoteSection),
  .short_len = TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES,
  .storage = __start_gnu_build_id_start,
  .sdk_version = TICOS_SDK_VERSION,
};
#else

// NOTE: We start the array with a 0x1, so the compiler will never place the variable in bss
TICOS_BUILD_ID_QUALIFIER uint8_t g_ticos_sdk_derived_build_id[TICOS_BUILD_ID_LEN] = { 0x1, };

TICOS_BUILD_ID_QUALIFIER sTicosBuildIdStorage g_ticos_build_id = {
  .type = kTicosBuildIdType_None,
  .len = sizeof(g_ticos_sdk_derived_build_id),
  .short_len = TICOS_EVENT_INCLUDED_BUILD_ID_SIZE_BYTES,
  .storage = g_ticos_sdk_derived_build_id,
  .sdk_version = TICOS_SDK_VERSION,
};
#endif
