//! @file
//!
//! @brief Tests for CDR implementation

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fakes/fake_ticos_platform_metrics_locking.h"
#include "fakes/fake_ticos_platform_time.h"

#include "ticos/core/custom_data_recording.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/compiler.h"
#include "ticos/core/math.h"
#include "ticos/config.h"

#include "ticos_custom_data_recording_private.h"

typedef struct {
  bool has_data;
  sTicosCdrMetadata metadata;
  const uint8_t *recording;
  size_t recording_len;
} sCdrFakeInfo;

static sCdrFakeInfo s_cdr_fake_info;

static void prv_set_fake_info(const sTicosCdrMetadata *metadata,
                              const void *recording, size_t recording_len) {
  s_cdr_fake_info = (sCdrFakeInfo) {
    .has_data = true,
    .metadata = *metadata,
    .recording = (const uint8_t *)recording,
    .recording_len = recording_len,
  };
}

static bool prv_has_cdr_cb(sTicosCdrMetadata *metadata) {
  if (!s_cdr_fake_info.has_data) {
    return false;
  }

  *metadata = s_cdr_fake_info.metadata;
  return true;
}

static bool prv_read_data_cb(uint32_t offset, void *buf, size_t buf_len) {
  assert((offset + buf_len) <= s_cdr_fake_info.recording_len);
  uint8_t *bufp = (uint8_t *)buf;
  memcpy(bufp, &s_cdr_fake_info.recording[offset], buf_len);
  return true;
}

static void prv_mark_cdr_read_cb(void) {
  s_cdr_fake_info = (sCdrFakeInfo) {
    .has_data = false,
  };
}

static bool prv_stub_has_cdr_cb(TICOS_UNUSED sTicosCdrMetadata *metadata) {
  return false;
}

static bool prv_stub_read_data_cb(TICOS_UNUSED uint32_t offset,
                                       TICOS_UNUSED void *buf,
                                       TICOS_UNUSED size_t buf_len) {
  return false;
}

static void prv_stub_mark_cdr_read_cb(void) { }

static sTicosCdrSourceImpl s_ticos_cdr_fake_source = {
  .has_cdr_cb = prv_has_cdr_cb,
  .read_data_cb = prv_read_data_cb,
  .mark_cdr_read_cb = prv_mark_cdr_read_cb,
};

static sTicosCdrSourceImpl s_ticos_cdr_stub_source = {
  .has_cdr_cb = prv_stub_has_cdr_cb,
  .read_data_cb = prv_stub_read_data_cb,
  .mark_cdr_read_cb = prv_stub_mark_cdr_read_cb,
};

static const uint8_t s_expected_encoded_buffer[] = {
  0xA7,
  0x02, 0x05,
  0x03, 0x01,
  0x0A, 0x64, 'm', 'a', 'i', 'n',
  0x09, 0x65, '1', '.', '2',  '.', '3',
  0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
  0x01, 0x14,
  0x04, 0xa4,
  // kTicosCdrInfoKey_DurationMs
  0x01, 0x00,
  // kTicosCdrInfoKey_Mimetypes
  0x02, 0x82, 0x6A, 't', 'e', 'x', 't', '/', 'p', 'l', 'a', 'i', 'n', 0x68, 't', 'e', 'x', 't', '/', 'c', 's', 'v',
  // kTicosCdrInfoKey_Reason
  0x03, 0x65, 'e', 'r', 'r', 'o', 'r',
  // kTicosCdrInfoKey_Data
  0x04, 0x4f, 0x6d, 0x65, 0x6d, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x72, 0x6f, 0x63, 0x6b, 0x73, 0x21, 0x0a
};
const size_t s_expected_encoded_size = sizeof(s_expected_encoded_buffer);


static const char *s_cdr_mimetypes[] = { TICOS_CDR_TEXT, TICOS_CDR_CSV };

static const uint8_t s_cdr_bin[] = {
  0x6d, 0x65, 0x6d, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x72, 0x6f, 0x63, 0x6b, 0x73, 0x21, 0x0a
};
static const sTicosCdrMetadata s_cdr_metadata = {
  .start_time = {
    .type = kTicosCurrentTimeType_UnixEpochTimeSec,
    .info = {
      .unix_timestamp_secs = 20,
    },
  },
  .mimetypes = s_cdr_mimetypes,
  .num_mimetypes = TICOS_ARRAY_SIZE(s_cdr_mimetypes),
  .data_size_bytes = sizeof(s_cdr_bin),
  .duration_ms = 0,
  .collection_reason = "error",
};

TEST_GROUP(TicosCdrSource) {
  void setup() {
    fake_ticos_platform_time_enable(true);
  }
  void teardown() {
    ticos_cdr_source_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(TicosCdrSource, Test_NoSources) {
  size_t total_size;
  const bool has_data = g_ticos_cdr_source.has_more_msgs_cb(&total_size);
  CHECK_FALSE(has_data);

  uint8_t buf[16];
  const bool success = g_ticos_cdr_source.read_msg_cb(0, &buf, sizeof(buf));
  CHECK_FALSE(success);

  // no-op
  g_ticos_cdr_source.mark_msg_read_cb();
}

static void prv_run_encoder_test(void) {
  prv_set_fake_info(&s_cdr_metadata, s_cdr_bin, sizeof(s_cdr_bin));

  // it should be safe to call this function multiple times
  size_t total_size = 0;
  bool has_data;
  for (int i = 0; i < 2; i++) {
    has_data = g_ticos_cdr_source.has_more_msgs_cb(&total_size);
    CHECK_TRUE(has_data);
    LONGS_EQUAL(s_expected_encoded_size, total_size);
  }

  uint8_t cbor_buffer[total_size];
  memset(cbor_buffer, 0x0, total_size);

  bool success = g_ticos_cdr_source.read_msg_cb(0, &cbor_buffer[0], total_size);
  CHECK_TRUE(success);
  MEMCMP_EQUAL(s_expected_encoded_buffer, cbor_buffer, s_expected_encoded_size);

  // same check but now do byte-wise reading
  memset(cbor_buffer, 0x0, total_size);
  for (size_t i = 0; i < total_size; i++) {
    success = g_ticos_cdr_source.read_msg_cb(i, &cbor_buffer[i], 1);
    CHECK_TRUE(success);
  }
  MEMCMP_EQUAL(s_expected_encoded_buffer, cbor_buffer, s_expected_encoded_size);

  // same check but now do 2 bytes at a time
  memset(cbor_buffer, 0x0, total_size);
  for (size_t i = 0; i < total_size; i += 2) {
    const size_t bytes_to_read = TICOS_MIN(2, total_size - i);
    success = g_ticos_cdr_source.read_msg_cb(i, &cbor_buffer[i], bytes_to_read);
    CHECK_TRUE(success);
  }
  MEMCMP_EQUAL(s_expected_encoded_buffer, cbor_buffer, s_expected_encoded_size);

  g_ticos_cdr_source.mark_msg_read_cb();
  // there should no longer be data
  has_data = g_ticos_cdr_source.has_more_msgs_cb(&total_size);
  CHECK_FALSE(has_data);
}

TEST(TicosCdrSource, Test_BasicCapture) {
  ticos_cdr_register_source(&s_ticos_cdr_fake_source);
  prv_run_encoder_test();
}

TEST(TicosCdrSource, Test_MultipleSources) {
  ticos_cdr_register_source(&s_ticos_cdr_stub_source);
  ticos_cdr_register_source(&s_ticos_cdr_fake_source);
  prv_run_encoder_test();
}

TEST(TicosCdrSource, Test_SourceRegistryFull) {
  for (size_t i = 0; i < TICOS_CDR_MAX_DATA_SOURCES; i++) {
    ticos_cdr_register_source(&s_ticos_cdr_stub_source);
  }

  // shouldn't get registered so there should be no data
  ticos_cdr_register_source(&s_ticos_cdr_fake_source);
  prv_set_fake_info(&s_cdr_metadata, s_cdr_bin, sizeof(s_cdr_bin));

  size_t total_size;
  const bool has_data = g_ticos_cdr_source.has_more_msgs_cb(&total_size);
  CHECK_FALSE(has_data);
}
