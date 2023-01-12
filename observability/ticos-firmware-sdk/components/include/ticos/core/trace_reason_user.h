#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Helpers to add user-defined reasons for "Trace Events"
//!
//! NOTE: The internals of the TICOS_TRACE_REASON APIs make use of "X-Macros" to enable more
//! flexibility improving and extending the internal implementation without impacting the externally
//! facing API.
//!
//! More details about error tracing in general can be found at https://ticos.io/error-tracing

#ifdef __cplusplus
extern "C" {
#endif

#include "ticos/core/compiler.h"
#include "ticos/config.h"

//! Defines a user-defined trace reason.
//!
//! User-defined error reasons are expected to be defined in a separate *.def file. Aside of
//! creating the file, you will need to add the preprocessor definition
//! TICOS_TRACE_REASON_USER_DEFS_FILE to point to the file. For example, passing the compiler
//! flag -DTICOS_TRACE_REASON_USER_DEFS_FILE=\"ticos_trace_reason_user_config.def\" indicates
//! that the file "ticos_trace_reason_user_config.def" will be included to pick up user-defined
//! error trace reasons. Please ensure the file can be found in the header search paths.
//!
//! The contents of the .def file could look like:
//!
//! // ticos_trace_reason_user_config.def
//! TICOS_TRACE_REASON_DEFINE(bluetooth_cmd_buffer_full)
//! TICOS_TRACE_REASON_DEFINE(sensor_ack_timeout)
//!
//! @param reason The name of reason, without quotes. This gets surfaced in the Ticos UI, so
//! it's useful to make these names human readable. C variable naming rules apply.
//! @note reason must be unique
#define TICOS_TRACE_REASON_DEFINE(reason) \
  kTcsTraceReasonUser_##reason,

//! Uses a user-defined trace reason. Before you can use a user-defined trace reason, it should
//! defined using TICOS_TRACE_REASON_DEFINE in ticos_trace_reason_user_config.def
//! @param reason The name of the reason, without quotes, as defined using
//! TICOS_TRACE_REASON_DEFINE.
#define TICOS_TRACE_REASON(reason) \
  kTcsTraceReasonUser_##reason

//! If trace events are not being used, including the user config file can be disabled
//! by adding -DTICOS_DISABLE_USER_TRACE_REASONS=1 to CFLAGs
#if !defined(TICOS_DISABLE_USER_TRACE_REASONS)
#define TICOS_DISABLE_USER_TRACE_REASONS 0
#endif /* TICOS_DISABLE_USER_TRACE_REASONS */

//! For compilers which support the __has_include macro display a more friendly error message
//! when the user defined header is not found on the include path
//!
//! NB: ARMCC and IAR define __has_include but they don't work as expected
# if !TICOS_DISABLE_USER_TRACE_REASONS
#  if !defined(__CC_ARM) && !defined(__ICCARM__)
#   if defined(__has_include) && !__has_include(TICOS_TRACE_REASON_USER_DEFS_FILE)
#     pragma message("ERROR: " TICOS_EXPAND_AND_QUOTE(TICOS_TRACE_REASON_USER_DEFS_FILE) " must be in header search path")
#     error "See trace_reason_user.h for more details"
#   endif
#  endif
# endif

typedef enum TcsTraceReasonUser {
  TICOS_TRACE_REASON_DEFINE(Unknown)

#if (TICOS_DISABLE_USER_TRACE_REASONS == 0)
  // Pick up any user specified trace reasons
  #include TICOS_TRACE_REASON_USER_DEFS_FILE
#endif

  // A precanned reason which is used by the "demo" component
  // (ticos_demo_cli_cmd_trace_event.c) and can be used for a user test command as well.
  TICOS_TRACE_REASON_DEFINE(TicosCli_Test)

  kTcsTraceReasonUser_NumReasons,
} eTcsTraceReasonUser;

#ifdef __cplusplus
}
#endif
