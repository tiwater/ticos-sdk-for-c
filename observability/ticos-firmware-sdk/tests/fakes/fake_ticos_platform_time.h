#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/core/platform/system_time.h"

#ifdef __cplusplus
extern "C" {
#endif

void fake_ticos_platform_time_enable(bool enable);
void fake_ticos_platform_time_set(const sTicosCurrentTime *time);

#ifdef __cplusplus
}
#endif
