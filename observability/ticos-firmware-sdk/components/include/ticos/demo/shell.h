#pragma once

//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Minimal shell/console implementation for platforms that do not include one.
//! NOTE: For simplicity, ANSI escape sequences are not dealt with!

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TicosShellImpl {
  //! Function to call whenever a character needs to be sent out.
  int (*send_char)(char c);
} sTicosShellImpl;

//! Initializes the demo shell. To be called early at boot.
void ticos_demo_shell_boot(const sTicosShellImpl *impl);

//! Call this when a character is received. The character is processed synchronously.
void ticos_demo_shell_receive_char(char c);

#ifdef __cplusplus
}
#endif
