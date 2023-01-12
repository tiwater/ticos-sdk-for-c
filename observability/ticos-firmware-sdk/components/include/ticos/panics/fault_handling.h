#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Handlers for faults & exceptions that are included in the Ticos SDK.

#include <inttypes.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/reboot_reason_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Additional information supplied to the Ticos extended assert handler
typedef struct TicosAssertInfo {
  uint32_t extra;
  eTicosRebootReason assert_reason;
} sTicosAssertInfo;

#if TICOS_COMPILER_ARM

//! Function prototypes for fault handlers

//! Non-Maskable Interrupt handler for ARM processors. The handler will capture fault
//! information and PC/LR addresses, trigger a coredump to be captured and finally reboot.
//!
//! The NMI is the highest priority interrupt that can run on ARM devices and as the name implies
//! cannot be disabled. Any fault from the NMI handler will trigger the ARM "lockup condition"
//! which results in a reset.
//!
//! Usually a system will reboot or enter an infinite loop if an NMI interrupt is pended so we
//! recommend overriding the default implementation with the Ticos Handler so you can discover
//! if that ever happens.
//!
//! However, a few RTOSs and vendor SDKs make use of the handler. If you encounter a duplicate
//! symbol name conflict due to this the ticos implementation can be disabled as follows:
//!   CFLAGS += -DTICOS_EXC_HANDLER_NMI=TicosNmi_Handler_Disabled
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_NMI(void);

//! Hard Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_HARD_FAULT(void);

//! Memory Management handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_MEMORY_MANAGEMENT(void);

//! Bus Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_BUS_FAULT(void);

//! Usage Fault handler for ARM processors. The handler will capture fault information
//! and PC/LR addresses, trigger a coredump to be captured and finally reboot.
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_USAGE_FAULT(void);

//! (Optional) interrupt handler which can be installed for a watchdog
//!
//! If a Watchdog Peripheral supports an early wakeup interrupt or a timer peripheral
//! has been configured as a "software" watchdog, this function should be used as
//! the interrupt handler.
//!
//! For more ideas about configuring watchdogs in general check out:
//!   https://ticos.io/root-cause-watchdogs
TICOS_NAKED_FUNC void TICOS_EXC_HANDLER_WATCHDOG(void);
#endif

//! Runs the Ticos assert handler.
//!
//! This should be the last function called as part of an a assert. Upon completion
//! it will reboot the system. Normally, this function is used via the
//! TICOS_ASSERT_RECORD and TICOS_ASSERT macros that automatically passes the program
//! counter and return address.
//!
//! @param pc The program counter
//! @param lr The return address
//! @see TICOS_ASSERT_RECORD
//! @see TICOS_ASSERT
#if defined(__CC_ARM) || (defined(__clang__) && defined(__ti__))
//! ARMCC and tiarmclang will optimize away link register stores from callsites which makes it
//! impossible for a reliable backtrace to be resolved so we don't use the NORETURN attribute
#else
TICOS_NORETURN
#endif
void ticos_fault_handling_assert(void *pc, void *lr);

//! Ticos assert handler with additional information.
//!
//! @param pc The program counter
//! @param lr The return address
//! @param extra_info Additional information used by the assert handler
//! @see TICOS_ASSERT_RECORD
//! @see TICOS_ASSERT
#if defined(__CC_ARM) || (defined(__clang__) && defined(__ti__))
//! ARMCC and tiarmclang optimize away link register stores from callsites which makes it
//! impossible for a reliable backtrace to be resolved so we don't use the NORETURN attribute
#else
TICOS_NORETURN
#endif
void ticos_fault_handling_assert_extra(void *pc, void *lr, sTicosAssertInfo *extra_info);

//! Handler called in all system-specific fault_handlers, to perform generic
//! bookkeeping or other operations shared by all fault handlers.
void ticos_fault_handling_common(void);

#ifdef __cplusplus
}
#endif
