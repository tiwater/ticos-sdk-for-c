#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Infra for collecting backtraces which can be parsed by ticos!

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "ticos/config.h"

#include "ticos/core/reboot_reason_types.h"
#include "ticos/panics/platform/coredump.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosCoredumpSaveInfo {
  const void *regs;
  size_t regs_size;
  eTicosRebootReason trace_reason;
  const sTcsCoredumpRegion *regions;
  size_t num_regions;
} sTicosCoredumpSaveInfo;

//! Invoked by assert handler to capture coredump.
//!
//! @note A user of the SDK shouldn't need to invoke this directly
//!
//! @param sTicosCoredumpSaveInfo Architecture specific information to save with the coredump
//! @return true if the coredump was saved and false if the save failed
bool ticos_coredump_save(const sTicosCoredumpSaveInfo *save_info);

//! Handler to be invoked from fault handlers
//!
//! By default, the Ticos SDK will automatically call this function as part of
//! exception handling for the target architecture.
//!
//! @param regs The register state at the time of the fault occurred
//! @param reason The reason the fault occurred
void ticos_fault_handler(const sTcsRegState *regs, eTicosRebootReason reason);

//! First function called in "ticos_fault_handler".
//!
//! This routine exists so an end user can optionally extend the fault handler logic
//! or print metadata about the fault which occurred
//!
//! @note By default this is a weak function which behaves as a no-op.
extern void ticos_platform_fault_handler(const sTcsRegState *regs, eTicosRebootReason reason);

//! Checks that a coredump can fit in the platform storage allocated
//!
//! @return true if the check passes & false otherwise. On failure, an error message
//! is logged with information about how much more storage is needed.
bool ticos_coredump_storage_check_size(void);

//! Computes the size required to save a coredump on the system
//!
//! @note A user of the SDK can call this on boot to assert that
//! coredump storage is large enough to capture the regions specified
//!
//! @return The space required to save the coredump or 0 on error
//! (i.e no coredump regions defined, coredump storage of 0 size)
size_t ticos_coredump_storage_compute_size_required(void);

//! Queries whether a valid coredump is present in the coredump storage.
//!
//! @param total_size_out Upon returning from the function, the size of the coredump
//! in bytes has been written to the variable.
//! @return true when a valid coredump is present in the storage.
bool ticos_coredump_has_valid_coredump(size_t *total_size_out);

//
// Integration utilities
//
// We recommend using these to verify the integration of coredump storage. They are not intended
// to be included in release builds.
//

//! Runs tests on platform's coredump storage implementation to verify functionality
//!
//! @note Since coredumps are saved from an interrupt context, we recommend calling
//!  this test routine from an ISR or with interrupts disabled.
//! @note This routine records errors which will be dumped in
//!  ticos_coredump_storage_debug_test_finish() but does not do any logging itself since it
//!  runs from an ISR.
//!
//! @return true if checks passed, false otherwise.
bool ticos_coredump_storage_debug_test_begin(void);

//! Finishes platform coredump storage test and dumps info about any errors that occurred
//!
//! @note This function tests ticos_platform_coredump_storage_clear() which gets called
//! while the system is running.
//!
//! @return if the entire storage test was succesful. On error, information is dumped
//!  to the CLI for further debug.
bool ticos_coredump_storage_debug_test_finish(void);

#if TICOS_CACHE_FAULT_REGS
//! Defined in ticos_coredump_regions_armv7.c but needed for platform ports,
//! like Zephyr, this function will allow the port to capture the ARM fault
//! registers into a RAM buffer cache. The coredump region getter will
//! use the cached copy if TICOS_CACHE_FAULT_REGS is defined.
//!
//! @note the intent is to call this function before the port starts consuming
//! the register values by wrapping the port's fault handler and then chaining
//! to it after saving the registers.
void ticos_coredump_cache_fault_regs(void);
#endif

#ifdef __cplusplus
}
#endif
