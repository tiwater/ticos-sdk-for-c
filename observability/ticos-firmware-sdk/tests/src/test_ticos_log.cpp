//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fakes/fake_ticos_platform_metrics_locking.h"

#include "ticos/core/log.h"
#include "ticos/core/log_impl.h"
#include "ticos/core/compact_log_serializer.h"

#include "ticos_log_data_source_private.h"
#include "ticos_log_private.h"

static bool s_fake_data_source_has_been_triggered;
bool ticos_log_data_source_has_been_triggered(void) {
  return s_fake_data_source_has_been_triggered;
}

TEST_GROUP(TicosLog) {
  void setup() {
    fake_ticos_metrics_platorm_locking_reboot();
    s_fake_data_source_has_been_triggered = false;

    // we selectively enable ticos_log_handle_saved_callback() for certain tests
    mock().disable();
  }
  void teardown() {
    CHECK(fake_ticos_platform_metrics_lock_calls_balanced());
    ticos_log_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static void prv_run_header_check(uint8_t *log_entry,
                                 eTicosPlatformLogLevel expected_level,
                                 const char *expected_log,
                                 size_t expected_log_len) {
  // NOTE: Since sTcsRamLogEntry is serialized out, we manually check the header here instead of
  // sharing the struct definition to catch unexpected changes in the layout.
  LONGS_EQUAL(expected_level & 0x7, log_entry[0]);
  LONGS_EQUAL(expected_log_len & 0xff, log_entry[1]);
  MEMCMP_EQUAL(expected_log, &log_entry[2],expected_log_len);
}

static void prv_read_log_and_check(eTicosPlatformLogLevel expected_level,
                                   eTicosLogRecordType expected_type,
                                   const void *expected_log, size_t expected_log_len) {
  sTicosLog log;
  // scribble a bad pattern to make sure ticos_log_read inits things
  memset(&log, 0xa5, sizeof(log));
  const bool found_log = ticos_log_read(&log);
  CHECK(found_log);
  LONGS_EQUAL(expected_log_len, log.msg_len);

  if (expected_type == kTicosLogRecordType_Preformatted) {
    STRCMP_EQUAL((const char *)expected_log, log.msg);
  } else {
    MEMCMP_EQUAL(expected_log, log.msg, expected_log_len);
  }

  LONGS_EQUAL(expected_level, log.level);
  LONGS_EQUAL(expected_type, log.type);
}

TEST(TicosLog, Test_BadInit) {
  // should be no-ops
  ticos_log_boot(NULL, 10);
  ticos_log_boot((void*)0xbadcafe, 0);

  // calling any API while not enabled should have no effect
  ticos_log_save_preformatted(kTicosPlatformLogLevel_Error, "1", 1);
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Error, "%d", 11223344);

  sTicosLog log;
  const bool log_found = ticos_log_read(&log);
  CHECK(!log_found);
}

TEST(TicosLog, Test_TicosLogBasic) {
  uint8_t s_ram_log_store[20];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *my_log = "12345678";
  const size_t my_log_len = strlen(my_log);
  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  eTicosLogRecordType type = kTicosLogRecordType_Preformatted;
  ticos_log_save_preformatted(level, my_log, my_log_len);
  // Write a second log, to exercise the s_ticos_ram_logger.log_read_offset
  // book-keeping:
  ticos_log_save_preformatted(level, my_log, my_log_len);

  prv_run_header_check(s_ram_log_store, level, my_log, my_log_len);
  prv_run_header_check(&s_ram_log_store[10], level, my_log, my_log_len);

  // Read the two logs:
  prv_read_log_and_check(level, type, my_log, my_log_len);
  prv_read_log_and_check(level, type, my_log, my_log_len);

  // should be no more logs
  sTicosLog log;
  const bool log_found = ticos_log_read(&log);
  CHECK(!log_found);
}

TEST(TicosLog, Test_TicosLogOversize) {
  uint8_t s_ram_log_store[TICOS_LOG_MAX_LINE_SAVE_LEN + sizeof(sTcsRamLogEntry)];
  memset(s_ram_log_store, 0, sizeof(s_ram_log_store));
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  char my_log[TICOS_LOG_MAX_LINE_SAVE_LEN + 2];
  memset(my_log, 'A', sizeof(my_log));
  my_log[sizeof(my_log) - 1] = '\0';

  const eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  eTicosLogRecordType type = kTicosLogRecordType_Preformatted;
  ticos_log_save_preformatted(level, my_log, strlen(my_log));

  my_log[sizeof(my_log) - 2] = '\0';
  prv_read_log_and_check(level, type, my_log, TICOS_LOG_MAX_LINE_SAVE_LEN);
}

void ticos_log_handle_saved_callback(void) {
  mock().actualCall(__func__);
}

TEST(TicosLog, Test_TicosLog_GetRegions) {
  // try to get regions before init has been called
  sTicosLogRegions regions;
  bool found = ticos_log_get_regions(&regions);
  CHECK(!found);

  // now init and confirm we get the expected regions
  uint8_t s_ram_log_store[10];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  found = ticos_log_get_regions(&regions);
  CHECK(found);
  LONGS_EQUAL(s_ram_log_store, regions.region[1].region_start);
  LONGS_EQUAL(sizeof(s_ram_log_store), regions.region[1].region_size);

  // sanity check - first region should be sTcsRamLogger
  const uint8_t *tcs_ram_logger = (const uint8_t *)regions.region[0].region_start;
  LONGS_EQUAL(1, tcs_ram_logger[0]); // version == 1
  LONGS_EQUAL(1, tcs_ram_logger[1]); // enabled == 1
}

TEST(TicosLog, Test_TicosHandleSaveCallback) {
  uint8_t s_ram_log_store[10];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  mock().enable();
  const char *log0 = "log0";
  ticos_log_save_preformatted(kTicosPlatformLogLevel_Debug,
                                 log0, strlen(log0));
  // should have been filtered so nothing should be called
  mock().checkExpectations();

  mock().expectOneCall("ticos_log_handle_saved_callback");
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Info, "log2");
  mock().checkExpectations();

  mock().expectOneCall("ticos_log_handle_saved_callback");
  ticos_log_save_preformatted(kTicosPlatformLogLevel_Warning,
                                 log0, strlen(log0));
  mock().checkExpectations();
}

TEST(TicosLog, Test_TicosLogTruncation) {
  const size_t max_log_size = TICOS_LOG_MAX_LINE_SAVE_LEN;

  char long_log[max_log_size + 1 + 1 /* \0 */] = { 0 };
  memset(long_log, 'a', sizeof(long_log) - 1);
  LONGS_EQUAL(max_log_size + 1, strlen(long_log));

  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  uint8_t s_ram_log_store[TICOS_LOG_MAX_LINE_SAVE_LEN * 2];

  // both preformatted and formatted variants should wind up being max size
  memset(s_ram_log_store, 0x0, sizeof(s_ram_log_store));
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  ticos_log_save_preformatted(level, long_log, strlen(long_log));
  prv_run_header_check(s_ram_log_store, level, long_log, TICOS_LOG_MAX_LINE_SAVE_LEN);

  memset(s_ram_log_store, 0x0, sizeof(s_ram_log_store));
  ticos_log_reset();
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
  TICOS_LOG_SAVE(level, "%s", long_log);
  prv_run_header_check(s_ram_log_store, level, long_log, TICOS_LOG_MAX_LINE_SAVE_LEN);
}

TEST(TicosLog, Test_TicosLogExpireOldest) {
  uint8_t s_ram_log_store[10];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *log0 = "0"; // 3 bytes
  const char *log1 = "12"; // 4 bytes with header
  const char *log2 = "45678"; // 7 bytes with header
  const char *log3 = "a"; // 6 bytes with header, should span off 4-9
  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;

  ticos_log_save_preformatted(level, log0, strlen(log0));
  ticos_log_save_preformatted(level, log1, strlen(log1));
  ticos_log_save_preformatted(level, log2, strlen(log2));
  ticos_log_save_preformatted(level, log3, strlen(log3));

  prv_run_header_check(&s_ram_log_store[4], level, log3, strlen(log3));
}

TEST(TicosLog, Test_TicosLogFrozenBuffer) {
    // Test that when the log buffer is frozen (because the logging data source
    // is using it), logs will not be pruned when a log gets written that does
    // not fit in the remaining space:

    mock().enable();
    mock().expectOneCall("ticos_log_handle_saved_callback");

    uint8_t s_ram_log_store[10];
    ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

    const char *log0 = "45678"; // 7 bytes with header
    eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
    ticos_log_save_preformatted(level, log0, strlen(log0));

    s_fake_data_source_has_been_triggered = true;

    const char *log1 = "abcde"; // 7 bytes with header
    ticos_log_save_preformatted(level, log1, strlen(log1));

    prv_run_header_check(&s_ram_log_store[0], level, log0, strlen(log0));
}

TEST(TicosLog, Test_TicosLogBufTooLongForStorage) {
  uint8_t s_ram_log_store[5];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const uint8_t magic_pattern = 0xa5;
  memset(s_ram_log_store, magic_pattern, sizeof(s_ram_log_store));

  // a log that is simply too big for storage should not be a no-op
  TICOS_LOG_SAVE(kTicosPlatformLogLevel_Info, "%s", "more than 5 bytes");
  for (size_t i = 0; i < sizeof(s_ram_log_store); i++) {
    LONGS_EQUAL(magic_pattern, s_ram_log_store[i]);
  }
}

TEST(TicosLog, Test_LevelFiltering) {
  uint8_t s_ram_log_store[10] = { 0 };
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  const char *filtered_log = "1234";
  const size_t filtered_log_len = strlen(filtered_log);
  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Debug;
  ticos_log_save_preformatted(level, filtered_log, filtered_log_len);
  TICOS_LOG_SAVE(level, "%s", filtered_log);

  // Enable persisting debug logs
  ticos_log_set_min_save_level(kTicosPlatformLogLevel_Debug);
  const char *unfiltered_log = "woo";
  const size_t unfiltered_log_len = strlen(unfiltered_log);
  ticos_log_save_preformatted(level, unfiltered_log, unfiltered_log_len);
  TICOS_LOG_SAVE(level, "%s", unfiltered_log);

  prv_run_header_check(s_ram_log_store, level, unfiltered_log, unfiltered_log_len);
  prv_run_header_check(&s_ram_log_store[5], level, unfiltered_log, unfiltered_log_len);
}

TEST(TicosLog, Test_DroppedLogs) {
  uint8_t s_ram_log_store[13] = { 0 };
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  eTicosLogRecordType type = kTicosLogRecordType_Preformatted;
  const char *initial_log = "hi world!";
  const size_t initial_log_len = strlen(initial_log);
  ticos_log_save_preformatted(level, initial_log, initial_log_len);
  prv_read_log_and_check(level, type, initial_log, initial_log_len);

  for (int i = 0; i < 6; i++) {
    TICOS_LOG_SAVE(level, "MSG %d", i);
  }

  const char *expected_string = "... 5 messages dropped ...";
  prv_read_log_and_check(kTicosPlatformLogLevel_Warning, type, expected_string,
                         strlen(expected_string));

  const char *expected_msg5 = "MSG 5";
  prv_read_log_and_check(level, type, expected_msg5, strlen(expected_msg5));
}

static bool prv_log_entry_copy_callback(sTcsLogIterator *iter, size_t offset,
                                        const char *buf, size_t buf_len) {
  return mock()
    .actualCall(__func__)
    .withPointerParameter("iter", iter)
    .withUnsignedLongIntParameter("offset", offset)
    .withConstPointerParameter("buf", buf)
    .withUnsignedLongIntParameter("buf_len", buf_len)
    .returnBoolValueOrDefault(true);
}

static bool prv_iterate_callback(sTcsLogIterator *iter){
  mock().actualCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", iter->read_offset);
  ticos_log_iter_copy_msg(iter, prv_log_entry_copy_callback);
  return true;
}

TEST(TicosLog, Test_Iterate) {
  uint8_t s_ram_log_store[10] = {0};
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  const char *log0 = "0";
  const size_t log0_len = strlen(log0);
  ticos_log_save_preformatted(level, log0, log0_len);

  const char *log1 = "1";
  const size_t log1_len = strlen(log1);
  ticos_log_save_preformatted(level, log1, log1_len);

  sTcsLogIterator iterator = (sTcsLogIterator) {0};

  mock().enable();

  mock().expectOneCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", 0);

  mock().expectOneCall("prv_log_entry_copy_callback")
    .withPointerParameter("iter", &iterator)
    .withUnsignedLongIntParameter("offset", 0)
    .withConstPointerParameter("buf", &s_ram_log_store[2])
    .withUnsignedLongIntParameter("buf_len", 1);

  mock().expectOneCall("prv_iterate_callback")
    .withUnsignedLongIntParameter("read_offset", 3);

  mock().expectOneCall("prv_log_entry_copy_callback")
    .withPointerParameter("iter", &iterator)
    .withUnsignedLongIntParameter("offset", 0)
    .withConstPointerParameter("buf", &s_ram_log_store[5])
    .withUnsignedLongIntParameter("buf_len", 1);

  ticos_log_iterate(prv_iterate_callback, &iterator);
}

#if TICOS_COMPACT_LOG_ENABLE

static uint8_t s_fake_compact_log[] = {0x01, 0x02, 0x03, 0x04};

bool ticos_vlog_compact_serialize(sTicosCborEncoder *encoder,
                                     TICOS_UNUSED uint32_t log_id,
                                     TICOS_UNUSED uint32_t compressed_fmt,
                                     TICOS_UNUSED va_list args) {
  return ticos_cbor_join(encoder, s_fake_compact_log, sizeof(s_fake_compact_log));
}

TEST(TicosLog, Test_CompactLog) {
  uint8_t s_ram_log_store[20];
  ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));

  eTicosPlatformLogLevel level = kTicosPlatformLogLevel_Info;
  eTicosLogRecordType type = kTicosLogRecordType_Compact;
  ticos_compact_log_save(level, 0, 0);

  // Read the log:
  prv_read_log_and_check(level, type, s_fake_compact_log, sizeof(s_fake_compact_log));
}

#endif
