#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include "ticos/config.h"

#include "FreeRTOSConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

//! By default, The Ticos FreeRTOS port determines what memory allocation scheme to
//! use based on the configSUPPORT_STATIC_ALLOCATION & configSUPPORT_DYNAMIC_ALLOCATION defines
//! within FreeRTOSConfig.h.
//!
//! Alternatively, TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION can be added to CFLAGS
//! or "ticos_platform_config.h" to override this default behavior
#if !defined(TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION)

#if configSUPPORT_STATIC_ALLOCATION == 1
#define TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION 1
#elif configSUPPORT_DYNAMIC_ALLOCATION == 1
#define TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION 0
#else
// If a user hasn't explicitly set an allocation scheme, fallback to using FreeRTOS default
// (dynamic allocation)
#define TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION 0
#endif

#endif /* TICOS_FREERTOS_PORT_USE_STATIC_ALLOCATION */


//! Should be called prior to making any Ticos SDK calls
void ticos_freertos_port_boot(void);

#ifdef __cplusplus
}
#endif
