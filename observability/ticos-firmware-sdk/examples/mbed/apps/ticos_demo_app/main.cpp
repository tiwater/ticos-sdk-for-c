//! @file
//! @brief
//! main() runs in its own thread in the RTOS.  It runs a demo CLI
//! that provides commands for exercising the Mbed platform reference
//! implementation.  It also spins up a separate "blinky" thread that
//! is intended to make coredumps more interesting.

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/http/platform/http_client.h"

#include "blinky.h"
#include "cli.h"

// configure your project key in mbed_app.json
// find the key on ticos under settings > general
sTcsHttpClientConfig g_tcs_http_client_config = {
  .api_key = MBED_CONF_TICOS_PROJECT_API_KEY,
};

int main(void) {
  // show a boot message to make it obvious when we reboot
  TICOS_LOG_INFO("Ticos Mbed OS 5 demo app started...");

  // start an led blink thread to make coredumps more interesting
  blinky_init();

  // most of the demo is here
  cli_init();
  cli_forever();

  TICOS_UNREACHABLE;
}
