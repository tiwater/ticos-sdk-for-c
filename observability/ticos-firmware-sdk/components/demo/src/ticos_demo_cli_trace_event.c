//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI command that exercises the TICOS_TRACE_EVENT API, capturing a
//! Trace Event with the error reason set to "TicosDemoCli_Error".

#include "ticos/demo/cli.h"

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/trace_event.h"

int ticos_demo_cli_cmd_trace_event_capture(int argc, TICOS_UNUSED char *argv[]) {
  // For more information on user-defined error reasons, see
  // the TICOS_TRACE_REASON_DEFINE macro in trace_reason_user.h .
  TICOS_TRACE_EVENT_WITH_LOG(TicosCli_Test, "Example Trace Event. Num Args %d", argc);
  return 0;
}
