//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands that exercises the TICOS_LOG_... and
//! ticos_log_trigger_collection() APIs. See debug_log.h and log.h for more info.

#include "ticos/demo/cli.h"

#include "ticos/core/compiler.h"
#include "ticos/core/log.h"
#include "ticos/config.h"
#include "ticos/core/debug_log.h"

int ticos_demo_cli_cmd_test_log(TICOS_UNUSED int argc,
                                   TICOS_UNUSED char *argv[]) {
  TICOS_LOG_RAW("Raw log!");
  TICOS_LOG_DEBUG("Debug log!");
  TICOS_LOG_INFO("Info log!");
  TICOS_LOG_WARN("Warning log!");
  TICOS_LOG_ERROR("Error log!");

#if TICOS_SDK_LOG_SAVE_DISABLE
  // TICOS_LOGs are not written to the ram backed log buffer so do
  // it explicitly
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Debug, "Debug log!");
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Info, "Info log!");
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Warning, "Warning log!");
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Error, "Error log!");
#endif
  return 0;
}

int ticos_demo_cli_cmd_trigger_logs(TICOS_UNUSED int argc,
                                       TICOS_UNUSED char *argv[]) {
  ticos_log_trigger_collection();
  return 0;
}
