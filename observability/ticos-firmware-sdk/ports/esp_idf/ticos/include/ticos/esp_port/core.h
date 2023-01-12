#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Returns whether or not there is Ticos data to send
//!
//! This function is called by ticos_http_client_post_data() when data is being pushed to the
//! ticos service.
//!
//! There is a weak implementation defined by default which checks for locally collected ticos
//! data. If multiple MCUs are forwarding data to the ESP32 for transport, this function can
//! be overriden to check the sources for data as well.
//!
//! @return true if there is ticos data available to send, false otherwise
bool ticos_esp_port_data_available(void);

//! Fills buffer with a Ticos "chunk" when there is data available
//!
//! This function is called by ticos_http_client_post_data() when data is being pushed to the
//! ticos service.
//!
//! There is a weak implementation defined by default which checks for locally collected ticos
//! data. If multiple MCUs are forwarding data to the ESP32 for transport, this function can
//! be overriden to check the sources for data as well.
//!
//! @param[out] buf The buffer to copy data to be sent into
//! @param[in,out] buf_len The size of the buffer to copy data into. On return, populated
//! with the amount of data, in bytes, that was copied into the buffer.
//!
//! @return true if the buffer was filled, false otherwise
bool ticos_esp_port_get_chunk(void *buf, size_t *buf_len);

//! Intializes the Ticos system, and should be called one time at boot.
//!
//! Note: by default this is called from the system initialization sequence-
//! it's placed into the global constructor table during compilation, and
//! executed by the esp-idf initialization code. Optionally it can instead be
//! explictly called during application startup by setting
//! 'CONFIG_TICOS_AUTOMATIC_INIT=n' in the project Kconfig options.
void ticos_boot(void);

#ifdef __cplusplus
}
#endif
