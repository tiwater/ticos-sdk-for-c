#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Zephyr specific http utility for interfacing with Ticos HTTP utilities

#include <stddef.h>
#include <stdbool.h>

#include "ticos/ports/zephyr/http.h"

#ifdef __cplusplus
extern "C" {
#endif

//! ticos_nrfconnect_port_* names are now deprecated.
//!
//! This is a temporary change to map to the appropriate name but will be removed in
//! future releases

#define ticos_nrfconnect_port_install_root_certs ticos_zephyr_port_install_root_certs
#define ticos_nrfconnect_port_post_data ticos_zephyr_port_post_data
#define ticos_nrfconnect_port_ota_update   ticos_zephyr_port_ota_update

#ifdef __cplusplus
}
#endif
