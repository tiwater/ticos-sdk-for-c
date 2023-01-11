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

#define TICOS_PACKED __attribute__((packed))
#define TICOS_PACKED_STRUCT struct TICOS_PACKED
//! TICOS_NORETURN is only intended to be overridden in unit tests, if needed
#if !defined(TICOS_NORETURN)
#define TICOS_NORETURN __attribute__((noreturn))
#endif
#define TICOS_NAKED_FUNC __attribute__((naked))
#define TICOS_UNREACHABLE __builtin_unreachable()
#if defined(__clang__)
#define TICOS_NO_OPT __attribute__((optnone))
#else
#define TICOS_NO_OPT __attribute__((optimize("O0")))
#endif

#define TICOS_ALIGNED(x) __attribute__((aligned(x)))
#define TICOS_UNUSED __attribute__((unused))
#define TICOS_USED __attribute__((used))
#define TICOS_WEAK __attribute__((weak))
#define TICOS_PRINTF_LIKE_FUNC(a, b) __attribute__ ((format (printf, a, b)))

//! From https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html,
//!  If x is 0, the result is undefined.
#define TICOS_CLZ(a) ((a == 0) ? 32UL : (uint32_t)__builtin_clz(a))

#if defined(__arm__)
#  define TICOS_GET_LR(_a) _a = __builtin_return_address(0)
#  define TICOS_GET_PC(_a) __asm volatile ("mov %0, pc" : "=r" (_a))
#  define TICOS_BREAKPOINT(val) __asm volatile ("bkpt "#val)
#elif defined(__XTENSA__)
#  define TICOS_GET_LR(_a) _a = __builtin_return_address(0)
#  define TICOS_GET_PC(_a) _a = ({ __label__ _l; _l: &&_l;});
#  define TICOS_BREAKPOINT(val)  __asm__ ("break 0,0")
#elif defined(TICOS_UNITTEST) || defined(__APPLE__)  // Ticos iOS SDK also #includes this header
#  define TICOS_GET_LR(_a) _a = 0
#  define TICOS_GET_PC(_a) _a = 0
#  define TICOS_BREAKPOINT(val)  (void)0
#else
#  define TICOS_GET_LR(_a) _a = __builtin_return_address(0)
// Take advantage of "Locally Declared Labels" to get a PC
//   https://gcc.gnu.org/onlinedocs/gcc/Local-Labels.html#Local-Labels
#  define TICOS_GET_PC(_a) _a = ({ __label__ _l; _l: &&_l;});
#  define TICOS_BREAKPOINT(val)  (void)0
#endif /* defined(__GNUC__) && defined(__arm__) */

#if defined(__APPLE__) && defined(TICOS_UNITTEST)
// NB: OSX linker has slightly different placement syntax and requirements
//
// Notably,
// 1. Comma seperated where first item is top level section (i.e __DATA), i.e
//   __attribute__((section("__DATA," x)))
// 2. total length of name must be between 1-16 characters
//
// For now we just stub it out since unit tests don't make use of section locations.
#  define TICOS_PUT_IN_SECTION(x)
#else
#  define TICOS_PUT_IN_SECTION(x) __attribute__((section(x)))
#endif

#if defined(__cplusplus)
#  define TICOS_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#  define TICOS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

// Macro for disabling specific warnings. There must be quotes in the argument
// to expand correctly to eg _Pragma("GCC diagnostic ignored \"warning-name\"")
// Example:
// TICOS_DISABLE_WARNING("-Wunused-macros")
#if defined(__clang__)
#define TICOS_PRAGMA_PREFIX clang
#else
#define TICOS_PRAGMA_PREFIX GCC
#endif
#define TICOS_DISABLE_WARNING(warning) \
  _Pragma(TICOS_EXPAND_AND_QUOTE(TICOS_PRAGMA_PREFIX diagnostic ignored warning))

#ifdef __cplusplus
}
#endif
