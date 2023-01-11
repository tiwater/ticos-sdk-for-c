//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! This file contains an example implementation of the pseudocode included in the Ticos Docs
//! https://ticos.io/data-transport-example
//!
//! This CLI command can be used with the Ticos GDB command "ticos install_chunk_handler" to
//! "drain" chunks up to the Ticos cloud directly from gdb.
//!
//! This can be useful when working on integrations and initially getting a transport path in place.
//! (gdb) source $TICOS_SDK/scripts/ticos_gdb.py
//! (gdb) ticos install_chunk_handler -pk <YOUR_PROJECT_KEY>
//! or for more details
//! (gdb) ticos install_chunk_handler --help
//!
//! For more details see https://ticos.io/posting-chunks-with-gdb
//!

#include "ticos/demo/cli.h"

#include "ticos/config.h"
#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer.h"

// Note: We mark the function as weak so an end user can override this with a real implementation
// and we disable optimizations so the parameters don't get stripped away
TICOS_NO_OPT
TICOS_WEAK
void user_transport_send_chunk_data(TICOS_UNUSED void *chunk_data,
                                    TICOS_UNUSED size_t chunk_data_len) {
}

static bool prv_try_send_ticos_data(void) {
  // buffer to copy chunk data into
  uint8_t buf[TICOS_DEMO_CLI_USER_CHUNK_SIZE];
  size_t buf_len = sizeof(buf);
  bool data_available = ticos_packetizer_get_chunk(buf, &buf_len);
  if (!data_available ) {
    return false; // no more data to send
  }
  // send payload collected to chunks/ endpoint
  user_transport_send_chunk_data(buf, buf_len);
  return true;
}

int ticos_demo_drain_chunk_data(TICOS_UNUSED int argc,
                                   TICOS_UNUSED char *argv[]) {
  while (prv_try_send_ticos_data()) { }
  return 0;
}
