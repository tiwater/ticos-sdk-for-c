//! @file

#include <string.h>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "fakes/fake_ticos_event_storage.h"

#include "ticos/core/arch.h"
#include "ticos/core/compact_log_serializer.h"
#include "ticos/core/compiler.h"
#include "ticos/core/data_packetizer_source.h"
#include "ticos/core/event_storage.h"
#include "ticos/core/trace_event.h"
#include "ticos_trace_event_private.h"

bool ticos_arch_is_inside_isr(void) {
  return mock().actualCall(__func__).returnBoolValueOrDefault(false);
}

bool ticos_vlog_compact_serialize(sTicosCborEncoder *encoder,
                                     TICOS_UNUSED uint32_t log_id,
                                     TICOS_UNUSED uint32_t compressed_fmt,
                                     TICOS_UNUSED va_list args) {
  const uint8_t cbor[] = { 0x84, 0x08, 0x01, 0x02, 0x03 };
  return ticos_cbor_join(encoder, &cbor, sizeof(cbor));
}

static const sTicosEventStorageImpl *s_fake_event_storage_impl;

#define TICOS_TRACE_EVENT_WORST_CASE_SIZE_BYTES (59)

TEST_GROUP(TcsTraceEvent) {
  void setup() {
    static uint8_t s_storage[100];
    fake_ticos_event_storage_clear();
    s_fake_event_storage_impl = ticos_events_storage_boot(
        &s_storage, sizeof(s_storage));

    mock().strictOrder();
  }

  void teardown() {
    mock().checkExpectations();
    mock().clear();
    ticos_trace_event_reset();
  }
};

TEST(TcsTraceEvent, Test_BootNullStorage) {
  const int rv = ticos_trace_event_boot(NULL);
  CHECK_EQUAL(rv, -4);
}

TEST(TcsTraceEvent, Test_BootStorageTooSmall) {
  fake_ticos_event_storage_set_available_space(TICOS_TRACE_EVENT_WORST_CASE_SIZE_BYTES - 1);
  const int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(rv, -3);
}


TEST(TcsTraceEvent, Test_CaptureButStorageUninitialized) {
  const int rv = ticos_trace_event_capture(kTcsTraceReasonUser_Unknown, NULL, NULL);
  CHECK_EQUAL(rv, -1);
}


static void prv_run_capture_test(
    void *pc, void *lr, int32_t *status_code, size_t storage_size,
    size_t expected_encoded_size) {
  fake_ticos_event_storage_clear();
  fake_ticos_event_storage_set_available_space(storage_size);
  mock().expectOneCall("ticos_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  const bool expect_rollback = (storage_size < expected_encoded_size);
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);

  const int rv = (status_code == NULL) ?
      ticos_trace_event_capture(kTcsTraceReasonUser_test, pc, lr) :
      ticos_trace_event_with_status_capture(kTcsTraceReasonUser_test, pc, lr, *status_code);

  CHECK_EQUAL(expect_rollback ? -2 : 0, rv);
  mock().checkExpectations();
}

TEST(TcsTraceEvent, Test_CaptureOk_PcAndLr) {
  fake_ticos_event_storage_clear();
  const int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
      0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
      0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
  };

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    void *pc = (void *)0x12345678;
    void *lr = (void *)0xaabbccdd;
    prv_run_capture_test(pc, lr, NULL, i, sizeof(expected_data));
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(TcsTraceEvent, Test_CaptureOk_PcAndLrAndStatus) {
  fake_ticos_event_storage_clear();
  const int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
    0xA7,
    0x02, 0x02,
    0x03, 0x01,
    0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
    0x0A, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04,
    0xA4,
    0x06, 0x03,
    0x02, 0x1A, 0x12, 0x34, 0x56, 0x78,
    0x03, 0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
    0x07, 0x20
  };

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    void *pc = (void *)0x12345678;
    void *lr = (void *)0xaabbccdd;
    int32_t status_code = -1;
    prv_run_capture_test(pc, lr, &status_code, i, sizeof(expected_data));
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

#if !TICOS_COMPACT_LOG_ENABLE

TEST(TcsTraceEvent, Test_CaptureOk_PcAndLrAndLog) {
  fake_ticos_event_storage_clear();
  int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
    0xA7,
    0x02, 0x02,
    0x03, 0x01,
    0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
    0x0A, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04,
    0xA4,
    0x06, 0x03,
    0x02, 0x1A, 0x12, 0x34, 0x56, 0x78,
    0x03, 0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
    0x08, 0x4E, '1', '2', '3','4','5','6','7','8','9','a', 'b', 'c', 'd', 'e'
  };

  void *pc = (void *)0x12345678;
  void *lr = (void *)0xaabbccdd;

#if defined(TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED) && !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  const size_t expected_check_isr_calls = 2;
#else
  const size_t expected_check_isr_calls = 1;
#endif

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    const size_t storage_size = i;
    fake_ticos_event_storage_clear();
    fake_ticos_event_storage_set_available_space(storage_size);
    mock().expectNCalls(expected_check_isr_calls, "ticos_arch_is_inside_isr");
    mock().expectOneCall("prv_begin_write");
    const bool expect_rollback = (storage_size < sizeof(expected_data));
    mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);

    rv = ticos_trace_event_with_log_capture(
        kTcsTraceReasonUser_test, pc, lr, "%d%d%d%d%d%d789abcdetruncated", 1, 2, 3, 4, 5, 6);

    CHECK_EQUAL(expect_rollback ? -2 : 0, rv);
    mock().checkExpectations();
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(TcsTraceEvent, Test_CaptureOk_PcAndLrAndLogFromIsr) {
  const uint8_t expected_data[] = {
    0xA7,
    0x02, 0x02,
    0x03, 0x01,
    0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
    0x0A, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04,
    0xA4,
    0x06, 0x03,
    0x02, 0x1A, 0x12, 0x34, 0x56, 0x78,
    0x03, 0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
    0x08, 0x4E, '1', '2', '3','4','5','6','7','8','9','a', 'b', 'c', 'd', 'e'
  };

  void *pc = (void *)0x12345678;
  void *lr = (void *)0xaabbccdd;

#if defined(TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED) && !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  const size_t expected_check_isr_calls = 2;
#else
  const size_t expected_check_isr_calls = 1;
#endif

  // let's also make sure collection from an isr works as expected
  fake_ticos_event_storage_clear();
  int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);
  fake_ticos_event_storage_set_available_space(sizeof(expected_data));

  mock().expectNCalls(expected_check_isr_calls, "ticos_arch_is_inside_isr").andReturnValue(true);
  rv = ticos_trace_event_with_log_capture(
      kTcsTraceReasonUser_test, pc, lr, "%d%d%d%d%d%d789abcdetruncated", 1, 2, 3, 4, 5, 6);
  CHECK_EQUAL(0, rv);

  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", false);
  rv = ticos_trace_event_try_flush_isr_event();
  CHECK_EQUAL(0, rv);
  mock().checkExpectations();

  size_t expected_isr_data_size = sizeof(expected_data);
#if defined(TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED) && !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  expected_isr_data_size -= 16;
  uint8_t expected_data_no_log_from_isr[expected_isr_data_size];
  memcpy(expected_data_no_log_from_isr, expected_data,  expected_isr_data_size);
  expected_data_no_log_from_isr[38] = 0xA3; // array of 3 items instead of 4

  fake_event_storage_assert_contents_match(expected_data_no_log_from_isr,
                                           sizeof(expected_data_no_log_from_isr));
#else
  fake_event_storage_assert_contents_match(expected_data, expected_isr_data_size);
#endif
}

#else

TEST(TcsTraceEvent, Test_CaptureOk_PcAndLrAndCompactLog) {
  fake_ticos_event_storage_clear();
  int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
    0xA7,
    0x02, 0x02,
    0x03, 0x01,
    0x07, 0x69, 'D', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D',
    0x0A, 0x64, 'm', 'a', 'i', 'n',
    0x09, 0x65, '1', '.', '2', '.', '3',
    0x06, 0x66, 'e', 'v', 't', '_', '2', '4',
    0x04,
    0xA3,
    0x06, 0x03,
    0x03, 0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
    0x09, 0x84, 0x08, 0x01, 0x02, 0x03,
  };

  void *lr = (void *)0xaabbccdd;

#if defined(TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED) && !TICOS_TRACE_EVENT_WITH_LOG_FROM_ISR_ENABLED
  const size_t expected_check_isr_calls = 2;
#else
  const size_t expected_check_isr_calls = 1;
#endif

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    const size_t storage_size = i;
    fake_ticos_event_storage_clear();
    fake_ticos_event_storage_set_available_space(storage_size);
    mock().expectNCalls(expected_check_isr_calls, "ticos_arch_is_inside_isr");
    mock().expectOneCall("prv_begin_write");
    const bool expect_rollback = (storage_size < sizeof(expected_data));
    mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);

    rv = ticos_trace_event_with_compact_log_capture(
        kTcsTraceReasonUser_test, lr, 0x40, 8, 1, 2, 3);

    CHECK_EQUAL(expect_rollback ? -2 : 0, rv);
    mock().checkExpectations();
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

#endif /* TICOS_COMPACT_LOG_ENABLE */

static void prv_setup_isr_test(void) {
  fake_ticos_event_storage_clear();
  int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  mock().expectOneCall("ticos_arch_is_inside_isr").andReturnValue(true);
  void *pc = (void *)0x12345678;
  void *lr = (void *)0xaabbccdd;
  rv = ticos_trace_event_capture(kTcsTraceReasonUser_test, pc, lr);
  CHECK_EQUAL(0, rv);
  mock().checkExpectations();

  mock().expectOneCall("ticos_arch_is_inside_isr").andReturnValue(true);
  void *pc2 = (void *)2;
  void *lr2 = (void *)1;
  rv = ticos_trace_event_capture(kTcsTraceReasonUser_test, pc2, lr2);
  CHECK_EQUAL(-2, rv);
  mock().checkExpectations();
}

TEST(TcsTraceEvent, Test_CaptureOk_FromIsrWithForceFlush) {
  prv_setup_isr_test();

  mock().expectOneCall("prv_begin_write");
  const bool expect_rollback = false;
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  const int rv = ticos_trace_event_try_flush_isr_event();
  CHECK_EQUAL(0, rv);
  mock().checkExpectations();

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
      0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
      0x1A, 0xAA, 0xBB, 0xCC, 0xDD,
  };

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(TcsTraceEvent, Test_CaptureOk_FromIsrWithLazyFlush) {
  prv_setup_isr_test();

  bool expect_rollback = true;
  fake_ticos_event_storage_set_available_space(10);
  mock().expectOneCall("ticos_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  void *pc2 = (void *)NULL;
  void *lr2 = (void *)1;
  int rv = ticos_trace_event_capture(kTcsTraceReasonUser_test, pc2, lr2);
  CHECK_EQUAL(-2, rv);
  mock().checkExpectations();

  const uint8_t expected_data[] = {
    // event logged from ISR
    0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
    0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
    0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
    0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
    0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA3, 0x06,
    0x03, 0x02, 0x1A, 0x12, 0x34, 0x56, 0x78, 0x03,
    0x1A, 0xAA, 0xBB, 0xCC, 0xDD,

    // event not logged from ISR
    0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
    0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
    0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
    0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
    0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA2, 0x06,
    0x03, 0x03, 0x01,
  };

  fake_ticos_event_storage_set_available_space(sizeof(expected_data));

  expect_rollback = false;
  // first the ISR event should be flushed now that we aren't in an ISR
  mock().expectOneCall("ticos_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);

  // then the second event we want to record should be stored
  mock().expectOneCall("prv_begin_write");
  mock().expectOneCall("prv_finish_write").withParameter("rollback", expect_rollback);
  rv = ticos_trace_event_capture(kTcsTraceReasonUser_test, pc2, lr2);
  CHECK_EQUAL(0, rv);

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(TcsTraceEvent, Test_CaptureOk_LrOnly) {
  fake_ticos_event_storage_clear();
  const int rv = ticos_trace_event_boot(s_fake_event_storage_impl);
  CHECK_EQUAL(0, rv);

  const uint8_t expected_data[] = {
      0xA7, 0x02, 0x02, 0x03, 0x01, 0x07, 0x69, 0x44,
      0x41, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44,
      0x0A, 0x64, 0x6D, 0x61, 0x69, 0x6E, 0x09, 0x65,
      0x31, 0x2E, 0x32, 0x2E, 0x33, 0x06, 0x66, 0x65,
      0x76, 0x74, 0x5F, 0x32, 0x34, 0x04, 0xA2, 0x06,
      0x03, 0x03, 0x01,
  };

  // anything less than the expected size should fail to encode
  for (size_t i = 1; i <= sizeof(expected_data); i++) {
    void *pc = (void *)NULL;
    void *lr = (void *)1;
    prv_run_capture_test(pc, lr, NULL, i, sizeof(expected_data));
  }

  fake_event_storage_assert_contents_match(expected_data, sizeof(expected_data));
}

TEST(TcsTraceEvent, Test_CaptureStorageFull) {
  fake_ticos_event_storage_clear();
  fake_ticos_event_storage_set_available_space(TICOS_TRACE_EVENT_WORST_CASE_SIZE_BYTES);

  CHECK_EQUAL(ticos_trace_event_boot(s_fake_event_storage_impl), 0);
  fake_ticos_event_storage_set_available_space(0);
  mock().expectOneCall("ticos_arch_is_inside_isr");
  mock().expectOneCall("prv_begin_write");

  // Expect rollback!
  mock().expectOneCall("prv_finish_write").withParameter("rollback", true);

  const int rv = ticos_trace_event_capture(kTcsTraceReasonUser_test, 0, 0);
  CHECK_EQUAL(-2, rv);
}

TEST(TcsTraceEvent, Test_GetWorstCaseSerializeSize) {
  const size_t worst_case_size = ticos_trace_event_compute_worst_case_storage_size();
  LONGS_EQUAL(TICOS_TRACE_EVENT_WORST_CASE_SIZE_BYTES, worst_case_size);
}
