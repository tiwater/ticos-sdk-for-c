//! @file
//!
//! @brief

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "ticos/core/batched_events.h"


TEST_GROUP(TicosBatchedEvents){
  void setup() {
  }
  void teardown() {
  }
};


TEST(TicosBatchedEvents, Test_TicosBatchedHeader) {
  sTicosBatchedEventsHeader header;

  ticos_batched_events_build_header(1000000, &header);
  LONGS_EQUAL(5, header.length);
  const uint8_t cbor_enc_1000000[] = { 0x9a, 0x00, 0x0f, 0x42, 0x40 };
  MEMCMP_EQUAL(cbor_enc_1000000, header.data, header.length);

  ticos_batched_events_build_header(2, &header);
  LONGS_EQUAL(1, header.length);
  LONGS_EQUAL(0x82, header.data[0]);

  ticos_batched_events_build_header(0, &header);
  LONGS_EQUAL(0, header.length);

  ticos_batched_events_build_header(1, &header);
  LONGS_EQUAL(0, header.length);

}
