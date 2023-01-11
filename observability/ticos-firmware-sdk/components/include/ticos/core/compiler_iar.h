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

#define TICOS_PACKED __packed
#define TICOS_PACKED_STRUCT TICOS_PACKED struct
#define TICOS_NORETURN __noreturn
#define TICOS_NAKED_FUNC
#define TICOS_UNREACHABLE while (1)
#define TICOS_NO_OPT _Pragma("optimize=none")
#define TICOS_ALIGNED(x) _Pragma(TICOS_QUOTE(data_alignment=x))
#define TICOS_UNUSED
#define TICOS_USED __root
#define TICOS_WEAK __weak
#define TICOS_PRINTF_LIKE_FUNC(a, b)
#define TICOS_CLZ(a) __iar_builtin_CLZ(a)


#define TICOS_GET_LR(_a) __asm volatile ("mov %0, lr" : "=r" (_a))
#define TICOS_GET_PC(_a) __asm volatile ("mov %0, pc" : "=r" (_a))
#define TICOS_PUT_IN_SECTION(x) _Pragma(TICOS_QUOTE(location=x))
#define TICOS_BREAKPOINT(val) __asm volatile ("bkpt %0" : : "i"(val))

#define TICOS_STATIC_ASSERT(cond, msg) static_assert(cond, msg)

#define TICOS_DISABLE_WARNING(warning)

#ifdef __cplusplus
}
#endif
