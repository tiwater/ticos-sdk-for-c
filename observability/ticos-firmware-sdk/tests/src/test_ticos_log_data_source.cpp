//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fakes/fake_ticos_platform_metrics_locking.h"
#include "fakes/fake_ticos_platform_time.h"

#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/log.h"
#include "ticos/core/log_impl.h"

#include "ticos_log_data_source_private.h"
}

static uint8_t s_ram_log_store[64];

static sTicosCurrentTime s_current_time;

static void prv_time_inc(int secs) {
  s_current_time.info.unix_timestamp_secs += (uint64_t)secs;
  fake_ticos_platform_time_set(&s_current_time);
}

TEST_GROUP(TicosLogDataSource) {
  void setup() {
    fake_ticos_platform_time_enable(true);
    s_current_time = (sTicosCurrentTime) {
      .type = kTicosCurrentTimeType_UnixEpochTimeSec,
      .info = {
        .unix_timestamp_secs = 0,
      },
    };
    fake_ticos_platform_time_set(&s_current_time);
    memset(s_ram_log_store, 0, sizeof(s_ram_log_store));
    ticos_log_boot(s_ram_log_store, sizeof(s_ram_log_store));
    ticos_log_set_min_save_level(kTicosPlatformLogLevel_Debug);
  }
  void teardown() {
    ticos_log_data_source_reset();
    ticos_log_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static size_t prv_add_logs(void) {
  ticos_log_save_preformatted(
    kTicosPlatformLogLevel_Debug, "debug", strlen("debug"));
  ticos_log_save_preformatted(
    kTicosPlatformLogLevel_Info, "info", strlen("info"));
  ticos_log_save_preformatted(
    kTicosPlatformLogLevel_Warning, "warning", strlen("warning"));
  ticos_log_save_preformatted(
    kTicosPlatformLogLevel_Error, "error", strlen("error"));
  return 4;
}

static const size_t expected_encoded_size = 59;
static const uint8_t expected_encoded_buffer[expected_encoded_size] = {
  0xA7,
  0x02, 0x04,
  0x03, 0x01,
  0x0A, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1', '.', '2',  '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
  0x01, 0x00,
  0x04, 0x88,
  0x00,
  0x65, 'd', 'e', 'b', 'u', 'g',
  0x01,
  0x64, 'i', 'n', 'f', 'o',
  0x02,
  0x67, 'w', 'a', 'r', 'n', 'i', 'n', 'g',
  0x03,
  0x65, 'e', 'r', 'r', 'o', 'r',
};


TEST(TicosLogDataSource, Test_TriggerOnce) {
  prv_add_logs();

  ticos_log_trigger_collection();
  CHECK_TRUE(ticos_log_data_source_has_been_triggered());
  // Idempotent:
  ticos_log_trigger_collection();
}


TEST(TicosLogDataSource, Test_TriggerNoopWhenNoLogs) {
  ticos_log_trigger_collection();
  CHECK_FALSE(ticos_log_data_source_has_been_triggered());
}


TEST(TicosLogDataSource, Test_TriggerNoopWhenNoUnsentLogs) {
  prv_add_logs();
  ticos_log_trigger_collection();
  g_ticos_log_data_source.mark_msg_read_cb();

  // All logs in the buffer have been marked as sent. Triggering again should be a no-op:
  ticos_log_trigger_collection();
  CHECK_FALSE(ticos_log_data_source_has_been_triggered());
}


TEST(TicosLogDataSource, Test_HasMoreMsgsCbNotTriggered) {
  // Buffer contains a log, but ticos_log_trigger_collection() has not been called:
  prv_add_logs();
  size_t size = 0;
  CHECK_FALSE(g_ticos_log_data_source.has_more_msgs_cb(&size));
  LONGS_EQUAL(0, size);
}


TEST(TicosLogDataSource, Test_HasMoreMsgs) {
  prv_add_logs();

  ticos_log_trigger_collection();

  size_t size = 0;
  CHECK_TRUE(g_ticos_log_data_source.has_more_msgs_cb(&size));
  LONGS_EQUAL(expected_encoded_size, size);
}


TEST(TicosLogDataSource, Test_ReadMsg) {
  prv_add_logs();

  ticos_log_trigger_collection();

  // These logs will not get included, because they happened after the
  // ticos_log_trigger_collection() call:
  prv_add_logs();

  uint8_t cbor_buffer[expected_encoded_size] = {0 };
  for (size_t offset = 0; offset < expected_encoded_size; ++offset) {
    const uint8_t canary = 0x55;
    uint8_t byte[3] = {canary, 0x00, canary};
    CHECK_TRUE(g_ticos_log_data_source.read_msg_cb(offset, &byte[1], 1));

    // Check canary values to detect buffer overruns:
    BYTES_EQUAL(byte[0], canary);
    BYTES_EQUAL(byte[2], canary);
    cbor_buffer[offset] = byte[1];

    // The time encoded in the message should be the time when ticos_log_trigger_collection()
    // was called. Time passing while draining the data source should not affect the message:
    prv_time_inc(131);
  }

  MEMCMP_EQUAL(expected_encoded_buffer, cbor_buffer, expected_encoded_size);
}


TEST(TicosLogDataSource, Test_MarkMsgRead) {
  const size_t num_batch_logs_1 = prv_add_logs();
  LONGS_EQUAL(num_batch_logs_1, ticos_log_data_source_count_unsent_logs());

  ticos_log_trigger_collection();

  // These logs will not get included, because they happened after the
  // ticos_log_trigger_collection() call:
  const size_t num_batch_logs_2 = prv_add_logs();
  LONGS_EQUAL(num_batch_logs_1 + num_batch_logs_2,
              ticos_log_data_source_count_unsent_logs());

  g_ticos_log_data_source.mark_msg_read_cb();

  LONGS_EQUAL(num_batch_logs_2, ticos_log_data_source_count_unsent_logs());
}
