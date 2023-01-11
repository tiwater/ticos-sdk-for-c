//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fakes/fake_ticos_event_storage.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/serializer_helper.h"
#include "ticos/metrics/serializer.h"
#include "ticos/metrics/utils.h"

static const sTicosEventStorageImpl *s_fake_event_storage_impl;
#define FAKE_EVENT_STORAGE_SIZE 54

TEST_GROUP(TicosMetricsSerializer){
  void setup() {
     static uint8_t s_storage[FAKE_EVENT_STORAGE_SIZE];
     s_fake_event_storage_impl = ticos_events_storage_boot(
         &s_storage, sizeof(s_storage));
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

//
// For the purposes of our serialization test, we will
// just serialize 1 of each supported type
//

void ticos_metrics_heartbeat_iterate(TicosMetricIteratorCallback cb, void *ctx) {
  sTicosMetricInfo info = { 0 };
  // Note: info.key._impl is not needed for serialization so leaving blank

  info.type = kTicosMetricType_Unsigned;
  info.val.u32 = 1000;
  cb(ctx, &info);

  info.type = kTicosMetricType_Signed;
  info.val.i32 = -1000;
  cb(ctx, &info);

  info.type = kTicosMetricType_Timer;
  info.val.u32 = 1234;
  cb(ctx, &info);

  info.type = kTicosMetricType_String;
  // chosen to be exactly 16 bytes to match the max storage set in
  // tests/stub_includes/ticos_metrics_heartbeat_config.def
  #define SAMPLE_STRING "123456789abcde"
  uint8_t sample_string[sizeof(SAMPLE_STRING)];
  info.val.ptr = sample_string;
  memcpy(info.val.ptr, SAMPLE_STRING, sizeof(SAMPLE_STRING));
  cb(ctx, &info);
}

size_t ticos_metrics_heartbeat_get_num_metrics(void) {
  // if this fails, it means we need to add add a report for the new type
  // to the fake "ticos_metrics_heartbeat_iterate"
  LONGS_EQUAL(kTicosMetricType_NumTypes, 4);
  return kTicosMetricType_NumTypes;
}

TEST(TicosMetricsSerializer, Test_TicosMetricSerialize) {
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);

  ticos_metrics_heartbeat_serialize(s_fake_event_storage_impl);

  // {
  // "2": 1,
  // "3": 1,
  // "10": "main",
  // "9": "1.2.3",
  // "6": "evt_24",
  // "4": {
  //  "1": [ 1000, -1000, 1234, "123456789abcde" ]
  //  }
  // }
  const uint8_t expected_serialization[] = {
    0xa6,
    0x02, 0x01,
    0x03, 0x01,
    0x0a, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04, 0xa1, 0x01, 0x84, 0x19, 0x03, 0xe8, 0x39, 0x03, 0xe7, 0x19, 0x04, 0xd2, 0x6e,
      '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e',
  };

  fake_event_storage_assert_contents_match(expected_serialization, sizeof(expected_serialization));
}

TEST(TicosMetricsSerializer, Test_TicosMetricSerializeWorstCaseSize) {
  const size_t worst_case_storage = ticos_metrics_heartbeat_compute_worst_case_storage_size();
  LONGS_EQUAL(60, worst_case_storage);
}

TEST(TicosMetricsSerializer, Test_TicosMetricSerializeOutOfSpace) {
  // iterate over all buffer sizes less than the encoding we need
  // this should exercise all early exit paths
  for (size_t i = 0; i < FAKE_EVENT_STORAGE_SIZE - 1; i++) {
    fake_ticos_event_storage_clear();
    fake_ticos_event_storage_set_available_space(i);

    mock().expectOneCall("prv_begin_write");
    mock().expectOneCall("prv_finish_write").withParameter("rollback", true);

    ticos_metrics_heartbeat_serialize(s_fake_event_storage_impl);

    mock().checkExpectations();
  }

  uint32_t drops = ticos_serializer_helper_read_drop_count();
  LONGS_EQUAL(FAKE_EVENT_STORAGE_SIZE - 1, drops);

  drops = ticos_serializer_helper_read_drop_count();
  LONGS_EQUAL(0, drops);
}

TEST(TicosMetricsSerializer, Test_TicosMetricTypes) {
  //! These should never change so that the same value can
  //! always be used to recover the type on the server
  CHECK_EQUAL(0, kTicosMetricType_Unsigned);
  CHECK_EQUAL(1, kTicosMetricType_Signed);
  CHECK_EQUAL(2, kTicosMetricType_Timer);
  CHECK_EQUAL(3, kTicosMetricType_String);
  //! This can change if new types are appended to the enum
  //! but we assert here to remind us to add the new type
  //! to the check here
  CHECK_EQUAL(4, kTicosMetricType_NumTypes);
}
