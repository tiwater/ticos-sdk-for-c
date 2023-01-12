#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A subsystem which can (optionally) be used to trace _all_ reboots taking place on the system
//!
//! The Ticos "panics" component will automatically save coredumps anytime the system crashes.
//! However, it can sometimes be useful to track other types of reset reasons such as a software
//! initiated reset to complete an OTA, a brown out reset, a hardware watchdog reset, etc
//!
//! To track these types of resets, the "panics" SDK component also exposes a lightweight "reboot
//! tracking" module.  More details can be found in the function descriptions below or a
//! step-by-step setup tutorial is available at https://ticos.io/2QlOlgH
//!
//! A user may also (optionally) use two APIs for catching & reacting to reboot loops:
//!  ticos_reboot_tracking_reset_crash_count()
//!  ticos_reboot_tracking_get_crash_count()

#include <inttypes.h>
#include <stddef.h>

#include "ticos/core/event_storage.h"
#include "ticos/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Additional information that can optionally be collected at bootup and appended to the current
//! reset information
typedef struct BootupInfo {
  //! Most MCUs have an always-on register that will report why the device rebooted (i.e normal
  //! reset, brown out reset, watchdog, etc). This value can be provided here to attach the current
  //! value of the register to the reboot information or be 0 otherwise
  uint32_t reset_reason_reg;

  //! If the reason for the current reboot is not already tracked, this value will be used.
  //!
  //! @note This can useful in situations where no code executes from the main application prior to
  //! reboot (for example, a reset button is pressed or another MCU pulls power to the rail) but
  //! info is made available to the application after bootup as to why it was reset (i.e bootloader
  //! passes additional state about reset to main app).
  //!
  //! @note If there is not additional info available about the reset, this should be set to 0
  //! (kTcsRebootReason_Unknown).
  eTicosRebootReason reset_reason;
} sResetBootupInfo;

//! Helper structure for storing/retrieving the device's reboot reason
typedef struct TcsRebootType {
  //! Stores the reboot reason determined from hardware during the current boot
  eTicosRebootReason reboot_reg_reason;
  //! Stores the reboot reason as read from s_tcs_reboot_info. This could be set in
  //! the prior boot from either:
  //! * the application using ticos_reboot_tracking_mark_reset_imminent (fault handler, firmware
  //! update, etc)
  //! * a reason determined from the reboot register at bootup
  eTicosRebootReason prior_stored_reason;
} sTcsRebootReason;

//! Value used to determine state of reboot tracking data
#define TICOS_REBOOT_REASON_NOT_SET 0xffffffff

#define TICOS_REBOOT_TRACKING_REGION_SIZE 64

//! Sets the memory region used for reboot tracking.
//!
//! @note This region should _not_ initialized by your bootloader or application.
//!
//!    To achieve this behavior, some compilers have NOINIT directives or with GCC LD
//!    this behavior can be easily achieved with something like:
//!
//!    // In a C File
//!    #include "ticos/core/compiler.h"
//!    TICOS_PUT_IN_SECTION(".tcs_reboot_info")
//!    static uint8_t s_reboot_tracking[TICOS_REBOOT_TRACKING_REGION_SIZE];
//!
//!    // In device LD file
//!    NOINIT (rw) :  ORIGIN = <addr>, LENGTH = 0x20
//!    .noinit (NOLOAD): { KEEP(*(*.tcs_reboot_info)) } > NOINIT
//!
//! @note The size of the region should be TICOS_REBOOT_TRACKING_REGION_SIZE
//! @note This should be called once on bootup of the system prior to making any other
//!   reboot_tracking calls
//! @param start_addr The location where reboot tracking is located
//! @param bootup_info See struct for more details. Can be NULL if there is no info
//!  to provide
void ticos_reboot_tracking_boot(void *start_addr, const sResetBootupInfo *bootup_info);

typedef struct TcsRebootTrackingRegInfo {
  uint32_t pc;
  uint32_t lr;
} sTcsRebootTrackingRegInfo;

//! Flag that a reboot is about to take place
//!
//! This is automatically called by the Ticos API for fault handlers and when
//! ticos_fault_handling_assert() is invoked
//!
//! It can also be called for happy-path reboots such as a reboot due to a user clicking
//! a button or a reboot due to an OTA update taking place. It's up to the user of the SDK
//! to call the API in these scenarios
//! @param reboot_reason The reason for the reboot. See eTicosRebootReason for options
//! @param reg Register state at the time the reboot was initiated or NULL if no state is available
void ticos_reboot_tracking_mark_reset_imminent(eTicosRebootReason reboot_reason,
                                                  const sTcsRebootTrackingRegInfo *reg);

//! Collects recent reset info and pushes it to ticos_event_storage so that the data can
//! can be sent out using the Ticos data packetizer
//!
//! @param storage_impl The event storage implementation being used (returned from
//!   ticos_events_storage_boot())
//! @return 0 on success or if there was nothing to collect, error code otherwise
int ticos_reboot_tracking_collect_reset_info(const sTicosEventStorageImpl *storage_impl);

//! Compute the worst case number of bytes required to serialize Ticos data
//!
//! @return the worst case amount of space needed to serialize an event
size_t ticos_reboot_tracking_compute_worst_case_storage_size(void);

//! Get the current crash count
//!
//! Every time the device resets due to a Reason of Unknown or Error, the crash count
//! is incremented.  A user of the SDK may (optionally) use this information to determine
//! if the device is crash looping and if so take recovery action.
//!
//! @return crash count
size_t ticos_reboot_tracking_get_crash_count(void);

//! Reset the crash count to 0
void ticos_reboot_tracking_reset_crash_count(void);

//! Flags that a coredump has been collected as part of this reboot
//!
//! @note This is called by the "panics" component coredump integration automatically and should
//! never need to be called by an end user directly
void ticos_reboot_tracking_mark_coredump_saved(void);

//! Get the reported reboot reason from boot
//!
//! Each time the device boots, the reboot reason mapped from the platform reboot register is
//! stored. This can be used either by other subsystems or users of the SDK.
//!
//! @param reboot_reason Pointer to store the reboot reason from boot
//! @return 0 on success or 1 if the reboot reason is invalid
//! or the input parameter is NULL

int ticos_reboot_tracking_get_reboot_reason(sTcsRebootReason *reboot_reason);

//! Returns a boolean representing whether an unexpected reboot occurred from boot
//!
//! This function uses a reboot reason from a reboot register and the prior reboot reason (if
//! present) to determine if a reboot was unexpected.
//!
//! @param unexpected_reboot_occurred Pointer to store boolean marking an unexpected reboot
//! @return 0 on success, or 1 if the result is invalid or the input parameter is NULL
int ticos_reboot_tracking_get_unexpected_reboot_occurred(bool *unexpected_reboot_occurred);

#ifdef __cplusplus
}
#endif
