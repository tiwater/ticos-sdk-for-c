#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"


#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "ticos/config.h"
#include "ticos/core/math.h"
#include "ticos/demo/shell.h"

#include "ticos/demo/shell_commands.h"

static size_t s_num_chars_sent = 0;
static char s_chars_sent_buffer[1024] = {0};

static int prv_send_char(char c) {
  CHECK(s_num_chars_sent < sizeof(s_chars_sent_buffer));
  s_chars_sent_buffer[s_num_chars_sent++] = c;
  return 1;
}

static int prv_test_handler(int argc, char **argv) {
  MockActualCall &m = mock().actualCall(__func__);
  for (int i = 0; i < argc; i++) {
    char buffer[11] = {0};
    sprintf(buffer, "%d", i);
    m.withStringParameter(buffer, argv[i]);
  }
  return 0;
}

static const sTicosShellCommand s_ticos_shell_commands[] = {
    {"test", prv_test_handler, "test command"},
    {"help", ticos_shell_help_handler, "Lists all commands"},
};

const sTicosShellCommand *const g_ticos_shell_commands = s_ticos_shell_commands;
const size_t g_ticos_num_shell_commands = TICOS_ARRAY_SIZE(s_ticos_shell_commands);

static void prv_receive_str(const char *str) {
  for (size_t i = 0; i < strlen(str); ++i) {
    ticos_demo_shell_receive_char(str[i]);
  }
}

static void prv_reset_sent_buffer(void) {
  s_num_chars_sent = 0;
  memset(s_chars_sent_buffer, 0, sizeof(s_chars_sent_buffer));
}

TEST_GROUP(TcsDemoShell){
  void setup() {
    const sTicosShellImpl impl = {
        .send_char = prv_send_char,
    };
    ticos_demo_shell_boot(&impl);
    STRCMP_EQUAL("\r\ntcs> ", s_chars_sent_buffer);

    prv_reset_sent_buffer();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
    prv_reset_sent_buffer();
  }
};

TEST(TcsDemoShell, Test_TcsDemoShellEcho) {
  ticos_demo_shell_receive_char('h');
  ticos_demo_shell_receive_char('i');
  CHECK_EQUAL(2, s_num_chars_sent);
  STRCMP_EQUAL("hi", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEchoBackspace) {
  mock().expectOneCall("prv_test_handler")
        .withParameter("0", "test");
  prv_receive_str("x\x08test\n");
  STRCMP_EQUAL("x\x08\x20\x08test\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEnter) {
  ticos_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEnterCR) {
  ticos_demo_shell_receive_char('\r');
  ticos_demo_shell_receive_char('\r');
  STRCMP_EQUAL("\r\ntcs> \r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEnterCRLF) {
  ticos_demo_shell_receive_char('\r');
  ticos_demo_shell_receive_char('\n');
  ticos_demo_shell_receive_char('\r');
  ticos_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\ntcs> \r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEnterLF) {
  ticos_demo_shell_receive_char('\n');
  ticos_demo_shell_receive_char('\n');
  STRCMP_EQUAL("\r\ntcs> \r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellEnterLFCR) {
  ticos_demo_shell_receive_char('\n');
  ticos_demo_shell_receive_char('\r');
  ticos_demo_shell_receive_char('\n');
  ticos_demo_shell_receive_char('\r');
  STRCMP_EQUAL("\r\ntcs> \r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellUnknownCmd) {
  prv_receive_str("foo\n");
  STRCMP_EQUAL("foo\r\nUnknown command: foo\r\nType 'help' to list all commands\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellTestCmd) {
  mock().expectOneCall("prv_test_handler")
    .withParameter("0", "test")
    .withParameter("1", "123")
    .withParameter("2", "abc")
    .withParameter("3", "def")
    .withParameter("4", "g");
  prv_receive_str("test 123 abc    def g\n");
  STRCMP_EQUAL("test 123 abc    def g\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellStripLeadingSpaces) {
  mock().expectOneCall("prv_test_handler")
        .withParameter("0", "test");
  prv_receive_str("    test\n");
  STRCMP_EQUAL("    test\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellHelpCmd) {
  prv_receive_str("help\n");
  STRCMP_EQUAL("help\r\ntest: test command\r\nhelp: Lists all commands\r\ntcs> ", s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellRxBufferFull) {
  for (size_t i = 0; i < TICOS_DEMO_SHELL_RX_BUFFER_SIZE; ++i) {
    ticos_demo_shell_receive_char('X');
  }
  STRCMP_EQUAL(
      "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n" // TICOS_DEMO_SHELL_RX_BUFFER_SIZE + 1 X's
      "Unknown command: XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n" // TICOS_DEMO_SHELL_RX_BUFFER_SIZE X's
      "Type 'help' to list all commands\r\n"
      "tcs> ",
      s_chars_sent_buffer);
}

TEST(TcsDemoShell, Test_TcsDemoShellBackspaces) {
  mock().expectOneCall("prv_test_handler")
      .withParameter("0", "test")
      .withParameter("1", "1");
  prv_receive_str("\b\bnop\b\b\btest 1\n");
  mock().checkExpectations();

  // use a memcmp so we can "see" the backspaces
  MEMCMP_EQUAL("nop\b \b\b \b\b \btest 1\r\ntcs> ", s_chars_sent_buffer, s_num_chars_sent);
}

TEST(TcsDemoShell, Test_TcsDemoShellNotBooted) {
   const sTicosShellImpl impl = {
     .send_char = NULL,
   };
   ticos_demo_shell_boot(&impl);

  prv_receive_str("test 1\n");
  LONGS_EQUAL(0, s_num_chars_sent);
}
