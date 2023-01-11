#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Wrappers for common macros & compiler specifics
//!
//! Note: This file should never be included directly but rather picked up through the compiler.h
//! header

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#define TICOS_PACKED __attribute__((packed))
#define TICOS_PACKED_STRUCT struct TICOS_PACKED
#define TICOS_NORETURN __attribute__((noreturn))
#define TICOS_NAKED_FUNC __attribute__((naked))
#define TICOS_UNREACHABLE while(1)
#define TICOS_NO_OPT


#define TICOS_PUT_IN_SECTION(x) __attribute__((section(x)))
#define TICOS_ALIGNED(x) __attribute__((aligned(x)))
#define TICOS_UNUSED __attribute__((unused))
#define TICOS_USED __attribute__((used))
#define TICOS_WEAK __attribute__((weak))
#define TICOS_PRINTF_LIKE_FUNC(a, b) __attribute__ ((format (printf, a, b)))

#define TICOS_CLZ(a) ((a == 0) ? 32UL : (uint32_t)__clz(a))

// Compiler incorrectly thinks return value is missing for pure asm function
// disable the check for them
#pragma diag_push
#pragma diag_suppress 994

#pragma FUNC_CANNOT_INLINE(ticos_get_pc)
TICOS_NAKED_FUNC
static void * ticos_get_pc(void) {
  __asm(" mov r0, lr \n"
        " bx lr");
}

#pragma FUNC_CANNOT_INLINE(__get_PSP)
TICOS_NAKED_FUNC
static uint32_t __get_PSP(void) {
  __asm("   mrs r0, psp\n"
        "   bx lr");
}

#pragma diag_pop

//! __builtin_return_address() currently always returns no value (0) but
//! there is a ticket tracking a real implementation:
//!   https://sir.ext.ti.com/jira/browse/EXT_EP-9303
#define TICOS_GET_LR(_a) _a = __builtin_return_address(0)

//! TI Compiler has an intrinsic __curpc() but it does not work when compiling against Cortex-M
//! variants (i.e --silicon_version=7M4)
#define TICOS_GET_PC(_a)  _a = ticos_get_pc();

#define TICOS_BREAKPOINT(val) __asm(" bkpt #"#val)

#if defined(__cplusplus)
#  define TICOS_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#  define TICOS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

#define TICOS_DISABLE_WARNING(warning)

#ifdef __cplusplus
}
#endif
