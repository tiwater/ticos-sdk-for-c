#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Command definitions for the minimal shell/console implementation.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosShellCommand {
  const char *command;
  int (*handler)(int argc, char *argv[]);
  const char *help;
} sTicosShellCommand;

extern const sTicosShellCommand *const g_ticos_shell_commands;
extern const size_t g_ticos_num_shell_commands;

int ticos_shell_help_handler(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
