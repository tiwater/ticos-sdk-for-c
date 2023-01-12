#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Internal file that should never be included by a consumer of the SDK. See
//! "ticos/core/build_info.h" for details on how to leverage the build id.

#include <stddef.h>
#include <stdint.h>

#include "ticos/core/compiler.h"
#include "ticos/version.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Note: These structures and values are also used in $TICOS_FIRMWARE_SDK/scripts/fw_build_id.py
// Any change here will likely require an update to the script was well!
//

typedef enum {
  //! No Build ID present.
  kTicosBuildIdType_None = 1,
  //! Build Id which can be emitted by GCC/LLVM compilers (https://ticos.io/gnu-build-id)
  kTicosBuildIdType_GnuBuildIdSha1 = 2,
  //! Build Id Type patched in by $TICOS_FIRMWARE_SDK/scripts/fw_build_id.py
  kTicosBuildIdType_TicosBuildIdSha1 = 3,
} eTicosBuildIdType;

typedef struct {
  uint8_t type; // eTicosBuildIdType
  uint8_t len;
  // the length, in bytes, of the build id used when reporting data
  uint8_t short_len;
  uint8_t rsvd;
  const void *storage;
  const sTcsSdkVersion sdk_version;
} sTicosBuildIdStorage;

TICOS_STATIC_ASSERT(((offsetof(sTicosBuildIdStorage, type) == 0) &&
                        (offsetof(sTicosBuildIdStorage, short_len) == 2)),
                       "be sure to update fw_build_id.py!");

#if defined(TICOS_UNITTEST)
//! NB: For unit tests we want to be able to instrument the data in the test
//! so we drop the `const` qualifier
#define TICOS_BUILD_ID_QUALIFIER
#else
#define TICOS_BUILD_ID_QUALIFIER const
#endif

extern TICOS_BUILD_ID_QUALIFIER sTicosBuildIdStorage g_ticos_build_id;
extern TICOS_BUILD_ID_QUALIFIER uint8_t g_ticos_sdk_derived_build_id[];

//! The layout of a Note section in an ELF. This is how Build Id information is layed out when
//! using kTicosBuildIdType_GnuBuildIdSha1
typedef TICOS_PACKED_STRUCT {
  uint32_t namesz;
  uint32_t descsz;
  uint32_t type;
  char  namedata[];
} sTicosElfNoteSection;

#ifdef __cplusplus
}
#endif
