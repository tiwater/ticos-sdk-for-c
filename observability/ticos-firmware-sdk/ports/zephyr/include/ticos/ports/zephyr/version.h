#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief A little convenience header to assist in checks which can be run at compile time for
//! backward compatibility based on Zephyr version.

#ifdef __cplusplus
extern "C" {
#endif

#include <version.h>

//! Returns true if current Zephyr Kernel Version is greater than the one specified
//!
//! Three checks:
//!  - First check if major version is greater than the one specified
//!  - Next check if the major version matches and the minor version is greater
//!  - Finally check if we are on a development build that is greater than the version.  After a
//!    release is shipped, a PATCHLEVEL of 99 is used to indicate this. For example, a version of
//!    "1.7.99" means development for version "1.8.0".
#define TICOS_ZEPHYR_VERSION_GT(major, minor)                                   \
  ((KERNEL_VERSION_MAJOR > (major)) ||                                             \
   ((KERNEL_VERSION_MAJOR == (major)) && (KERNEL_VERSION_MINOR > (minor))) ||      \
   ((KERNEL_VERSION_MAJOR == (major)) && ((KERNEL_VERSION_MINOR + 1) > (minor)) && \
    (KERNEL_PATCHLEVEL == 99)))

#ifdef __cplusplus
}
#endif
