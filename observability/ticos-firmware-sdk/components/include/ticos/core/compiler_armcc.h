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
#define TICOS_NORETURN __declspec(noreturn)
#define TICOS_NAKED_FUNC __asm
#define TICOS_UNREACHABLE while (1)
#define TICOS_NO_OPT
#define TICOS_ALIGNED(x) __attribute__((aligned(x)))
#define TICOS_UNUSED __attribute__((unused))
#define TICOS_USED __attribute__((used))
#define TICOS_WEAK __attribute__((weak))
#define TICOS_PRINTF_LIKE_FUNC(a, b)
#define TICOS_CLZ(a) __clz(a)


#define TICOS_GET_LR(_a) _a = ((void *)__return_address())
#define TICOS_GET_PC(_a) _a = (void *)__current_pc()
#define TICOS_PUT_IN_SECTION(x) __attribute__((section(x), zero_init))
#define TICOS_BREAKPOINT(val) __breakpoint(val)

#define TICOS_STATIC_ASSERT(expr, msg) \
    enum {TICOS_CONCAT(TICOS_ASSERTION_AT_, __LINE__) = sizeof(char[(expr) ? 1 : -1])}

#define TICOS_DISABLE_WARNING(warning)

#ifdef __cplusplus
}
#endif
