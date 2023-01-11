//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Minimal shell/console implementation for platforms that do not include one.
//! NOTE: For simplicity, ANSI escape sequences are not dealt with!

#include "ticos/demo/shell.h"
#include "ticos/demo/shell_commands.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ticos/config.h"
#include "ticos/core/compiler.h"

#define TICOS_SHELL_MAX_ARGS (16)
#define TICOS_SHELL_PROMPT "tcs> "

#define TICOS_SHELL_FOR_EACH_COMMAND(command) \
  for (const sTicosShellCommand *command = g_ticos_shell_commands; \
    command < &g_ticos_shell_commands[g_ticos_num_shell_commands]; \
    ++command)

static struct TicosShellContext {
  int (*send_char)(char c);
  size_t rx_size;
  // the char we will ignore when received end-of-line sequences
  char eol_ignore_char;
  char rx_buffer[TICOS_DEMO_SHELL_RX_BUFFER_SIZE];
} s_tcs_shell;

static bool prv_booted(void) {
  return s_tcs_shell.send_char != NULL;
}

static void prv_send_char(char c) {
  if (!prv_booted()) {
    return;
  }
  s_tcs_shell.send_char(c);
}

static void prv_echo(char c) {
  if (c == '\n') {
    prv_send_char('\r');
    prv_send_char('\n');
  } else if ('\b' == c) {
    prv_send_char('\b');
    prv_send_char(' ');
    prv_send_char('\b');
  } else {
    prv_send_char(c);
  }
}

static char prv_last_char(void) {
  return s_tcs_shell.rx_buffer[s_tcs_shell.rx_size - 1];
}

static bool prv_is_rx_buffer_full(void) {
  return s_tcs_shell.rx_size >= TICOS_DEMO_SHELL_RX_BUFFER_SIZE;
}

static void prv_reset_rx_buffer(void) {
  memset(s_tcs_shell.rx_buffer, 0, sizeof(s_tcs_shell.rx_buffer));
  s_tcs_shell.rx_size = 0;
}

static void prv_echo_str(const char *str) {
  for (const char *c = str; *c != '\0'; ++c) {
    prv_echo(*c);
  }
}

static void prv_send_prompt(void) {
  prv_echo_str(TICOS_SHELL_PROMPT);
}

static const sTicosShellCommand *prv_find_command(const char *name) {
  TICOS_SHELL_FOR_EACH_COMMAND(command) {
    if (strcmp(command->command, name) == 0) {
      return command;
    }
  }
  return NULL;
}

static void prv_process(void) {
  if (prv_last_char() != '\n' && !prv_is_rx_buffer_full()) {
    return;
  }

  char *argv[TICOS_SHELL_MAX_ARGS] = {0};
  int argc = 0;

  char *next_arg = NULL;
  for (size_t i = 0; i < s_tcs_shell.rx_size && argc < TICOS_SHELL_MAX_ARGS; ++i) {
    char *const c = &s_tcs_shell.rx_buffer[i];
    if (*c == ' ' || *c == '\n' || i == s_tcs_shell.rx_size - 1) {
      *c = '\0';
      if (next_arg) {
        argv[argc++] = next_arg;
        next_arg = NULL;
      }
    } else if (!next_arg) {
      next_arg = c;
    }
  }

  if (s_tcs_shell.rx_size == TICOS_DEMO_SHELL_RX_BUFFER_SIZE) {
    prv_echo('\n');
  }

  if (argc >= 1) {
    const sTicosShellCommand *command = prv_find_command(argv[0]);
    if (!command) {
      prv_echo_str("Unknown command: ");
      prv_echo_str(argv[0]);
      prv_echo('\n');
      prv_echo_str("Type 'help' to list all commands\n");
    } else {
      command->handler(argc, argv);
    }
  }
  prv_reset_rx_buffer();
  prv_send_prompt();
}

void ticos_demo_shell_boot(const sTicosShellImpl *impl) {
  s_tcs_shell.eol_ignore_char = 0;
  s_tcs_shell.send_char = impl->send_char;
  prv_reset_rx_buffer();
  prv_echo_str("\n" TICOS_SHELL_PROMPT);
}

//! Logic to deal with CR, LF, CRLF, or LFCR end-of-line (EOL) sequences
//! @return true if the character should be ignored, false otherwise
static bool prv_should_ignore_eol_char(char c) {
  if (s_tcs_shell.eol_ignore_char != 0) {
    return (c == s_tcs_shell.eol_ignore_char);
  }

  //
  // Check to see if we have encountered our first newline character since the shell was booted
  // (either a CR ('\r') or LF ('\n')). Once found, we will use this character as our EOL delimiter
  // and ignore the opposite character if we see it in the future.
  //

  if (c == '\r') {
    s_tcs_shell.eol_ignore_char = '\n';
  } else if (c == '\n') {
    s_tcs_shell.eol_ignore_char = '\r';
  }
  return false;
}

void ticos_demo_shell_receive_char(char c) {
  if (prv_should_ignore_eol_char(c) || prv_is_rx_buffer_full() || !prv_booted()) {
    return;
  }

  const bool is_backspace = (c == '\b');
  if (is_backspace && s_tcs_shell.rx_size == 0) {
    return; // nothing left to delete so don't echo the backspace
  }

  // CR are our EOL delimiter. Remap as a LF here since that's what internal handling logic expects
  if (c == '\r') {
    c = '\n';
  }

  prv_echo(c);

  if (is_backspace) {
    s_tcs_shell.rx_buffer[--s_tcs_shell.rx_size] = '\0';
    return;
  }

  s_tcs_shell.rx_buffer[s_tcs_shell.rx_size++] = c;

  prv_process();
}

int ticos_shell_help_handler(TICOS_UNUSED int argc, TICOS_UNUSED char *argv[]) {
  TICOS_SHELL_FOR_EACH_COMMAND(command) {
    prv_echo_str(command->command);
    prv_echo_str(": ");
    prv_echo_str(command->help);
    prv_echo('\n');
  }
  return 0;
}
