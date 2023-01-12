#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stddef.h>
#include <string.h>

#include "ticos/core/data_export.h"
#include "ticos/core/data_packetizer.h"
#include "ticos/core/math.h"

static uint8_t s_fake_chunk1[] = { 0x1 };
static uint8_t s_fake_chunk2[TICOS_DATA_EXPORT_CHUNK_MAX_LEN];

typedef struct {
  void *data;
  size_t length;
} sTicosChunkTestData;

static const sTicosChunkTestData s_fake_chunks[] = {
  [0] = {
    .data = s_fake_chunk1,
    .length = sizeof(s_fake_chunk1),
  },
  [1] = {
    .data = s_fake_chunk2,
    .length = sizeof(s_fake_chunk2),
  }
};

static int s_current_chunk_idx = 0;

TEST_GROUP(TicosDataExport) {
  void setup() {
    for (size_t i = 0; i < sizeof(s_fake_chunk2); i++) {
      s_fake_chunk2[i] = i & 0xff;
    }
    s_current_chunk_idx = 0;
     mock().strictOrder();
  }

  void teardown() {
     mock().clear();
  }
};

void ticos_data_export_base64_encoded_chunk(const char *chunk_str) {
  mock().actualCall(__func__).withStringParameter("chunk_str", chunk_str);
  CHECK(strlen(chunk_str) < TICOS_DATA_EXPORT_BASE64_CHUNK_MAX_LEN);
}

bool ticos_packetizer_get_chunk(void *buf, size_t *buf_len) {
  const size_t num_chunks = TICOS_ARRAY_SIZE(s_fake_chunks);
  if (s_current_chunk_idx == num_chunks) {
    return false;
  }

  const sTicosChunkTestData *chunk = &s_fake_chunks[s_current_chunk_idx];
  s_current_chunk_idx++;

  LONGS_EQUAL(TICOS_DATA_EXPORT_CHUNK_MAX_LEN, *buf_len);
  CHECK(chunk->length <= *buf_len);

  memcpy(buf, chunk->data, chunk->length);
  *buf_len = chunk->length;

  return true;
}

TEST(TicosDataExport, Test_TicosDataExport) {
  mock().expectOneCall("ticos_data_export_base64_encoded_chunk")
      .withStringParameter("chunk_str", "MC:AQ==:");
  mock().expectOneCall("ticos_data_export_base64_encoded_chunk")
      .withStringParameter("chunk_str", "MC:AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk8=:");

  ticos_data_export_dump_chunks();

  mock().checkExpectations();
}
