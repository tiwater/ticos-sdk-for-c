#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! Ports to facilitate the integration of Ticos reboot reason tracking
//! For more details about the integration, see https://ticos.io/reboot-reasons

#include "ticos/core/reboot_tracking.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Reads platform reset reason registers and converts to format suitable for reporting to Ticos
//!
//! @note Reboot reasons are recorded and reported to the Ticos UI and
//! serve as a leading indicator to issues being seen in the field.
//! @note For MCUs where reset reason register information is "sticky" (persists across resets),
//! the platform port will clear the register. This way the next boot will be guaranteed to reflect
//! the correct information for the reboot that just took place.
//! @note By default, ports print the MCU reset information on bootup using the TICOS_LOG
//! infrastructure. This can optionally be disabled by adding -DTICOS_ENABLE_REBOOT_DIAG_DUMP=0
//! to your list of compiler flags
//!
//! @param[out] info Populated with platform specific reset reason info. Notably, reset_reason_reg
//!  is populated with the reset reason register value and reset_reason is populated with a
//!  eTicosRebootReason which will be displayed in the Ticos UI
void ticos_reboot_reason_get(sResetBootupInfo *info);

#ifdef __cplusplus
}
#endif
