//! @file
//! @brief
//! This demonstration CLI provides a way to interact with the Mbed
//! platform reference implementation by providing commands to crash
//! and get/delete/print the stored coredump.

#include "cli.h"

#include "mbed.h"
#include "ticos/core/debug_log.h"
#include "ticos/demo/cli.h"
#include "ticos/demo/shell.h"

static int prv_putchar_shim(char c) {
  return putchar((int)c);
}

void cli_init(void) {
  const sTicosShellImpl s_shell_impl = {
    .send_char = prv_putchar_shim
  };
  ticos_demo_shell_boot(&s_shell_impl);
}

TICOS_NORETURN void cli_forever(void) {
  while (true) {
    int c = getchar();
    if (c != EOF) {
      ticos_demo_shell_receive_char((char)c);
    }
  }

  TICOS_UNREACHABLE;
}
