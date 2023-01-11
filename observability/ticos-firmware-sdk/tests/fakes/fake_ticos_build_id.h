#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <stddef.h>

#include "ticos/core/build_info.h"
#include "ticos_build_id_private.h"

#ifdef __cplusplus
extern "C" {
#endif

extern eTicosBuildIdType g_fake_ticos_build_id_type;
extern sTicosBuildInfo g_fake_ticos_build_id_info;

void fake_ticos_build_id_reset(void);

#ifdef __cplusplus
}
#endif
