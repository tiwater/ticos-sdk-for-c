#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! @details
//!
//! @brief
//! Platform APIs used to get information about the device and its components.

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosDeviceInfo {
  //! The device's serial number or unique identifier.
  //! Regular expression defining valid device serials: ^[-a-zA-Z0-9_]+$
  const char *device_serial;

  //! The "Software Type" is used to identify the separate pieces of software running on a given device. This can be
  //! images running on different MCUs (i.e "main-mcu-app" & "bluetooth-mcu") or different images running on the same
  //! MCU (i.e "main-mcu-bootloader" & "main-mcu-app").
  //! To learn more, check out the documentation: https://ticos.io/34PyNGQ
  const char *software_type;

  //! Version of the currently running software.
  //! We recommend using Semantic Version V2 strings.
  const char *software_version;

  //! Hardware version (sometimes also called "board revision") that the software is currently running on.
  //! Regular expression defining valid hardware versions: ^[-a-zA-Z0-9_\.\+]+$
  const char *hardware_version;
} sTicosDeviceInfo;

//! Invoked by ticos library to query the device information
//!
//! It's expected the strings returned will be valid for the lifetime of the application
void ticos_platform_get_device_info(sTicosDeviceInfo *info);

//! Allows caller to get a pointer to a unique version string
//! starting with their supplied version. Will insert a plus
//! sign between the supplied version and the unique suffix
//! to help ensure Semantic (SemVer) compliance.
//!
//! @note The unique version string is generated using the configured build id
//! (https://ticos.io/unique-build-id)
//!
//! @return System version with suffix or null on error. For example,
//! if the input version is "1.0.0" and the build id is "123456....", the string
//! returned will be "1.0.0+123456"
const char *ticos_create_unique_version_string(const char * const version);

//! Convenience function to get a pointer to a previously created version string.
const char *ticos_get_unique_version_string(void);

#ifdef __cplusplus
}
#endif
