#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Wrappers for common macros & compiler specifics

#ifdef __cplusplus
extern "C" {
#endif

#define TICOS_QUOTE(str) #str
#define TICOS_EXPAND_AND_QUOTE(str) TICOS_QUOTE(str)
#define TICOS_CONCAT_(x, y) x##y
#define TICOS_CONCAT(x, y) TICOS_CONCAT_(x, y)

// Given a static string definition, compute the strlen equivalent
// (i.e TICOS_STATIC_STRLEN("abcd") == 4)
#define TICOS_STATIC_STRLEN(s) (sizeof(s) - 1)

// A convenience macro that can be checked to see if the current compiler being used targets an
// ARM-based architecture
#if defined(__arm__) || defined(__ICCARM__) || defined(__TI_ARM__)
#define TICOS_COMPILER_ARM 1
#else
#define TICOS_COMPILER_ARM 0
#endif

//
// Pick up compiler specific macro definitions
//

#if defined(__CC_ARM)
#include "compiler_armcc.h"
#elif defined(__TI_ARM__)
#include "compiler_ti_arm.h"
#elif defined(__GNUC__) || defined(__clang__)
#include "compiler_gcc.h"
#elif defined(__ICCARM__)
#include "compiler_iar.h"
#else
#  error "New compiler to add support for!"
#endif


#ifdef __cplusplus
}
#endif
