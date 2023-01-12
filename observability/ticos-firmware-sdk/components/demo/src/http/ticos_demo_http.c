//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! CLI commands which require integration of the "http" component.

#include "ticos/http/http_client.h"

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/errors.h"
#include "ticos/demo/cli.h"
#include "ticos/demo/util.h"

const char *ticos_demo_get_chunks_url(void) {
  static char s_chunks_url[TICOS_HTTP_URL_BUFFER_SIZE];
  ticos_http_build_url(s_chunks_url, TICOS_HTTP_CHUNKS_API_SUBPATH);
  return s_chunks_url;
}

const char *ticos_demo_get_api_project_key(void) {
  return g_tcs_http_client_config.api_key;
}

int ticos_demo_cli_cmd_post_core(TICOS_UNUSED int argc,
                                    TICOS_UNUSED char *argv[]) {
  TICOS_LOG_INFO("Posting Ticos Data...");
  return ticos_http_client_post_chunk();
}
