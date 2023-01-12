#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"

extern "C" {
  #include <string.h>
  #include <stddef.h>

  #include "ticos/util/crc16_ccitt.h"
}

TEST_GROUP(TcsCrc16CcittGroup){
  void setup() { }
  void teardown() { }
};

TEST(TcsCrc16CcittGroup, Test_TcsCrc16CcittGroupComputeCodewords) {
  const uint8_t codeword1[] = {0x54, 0x1A, 0x71};
  uint16_t crc = ticos_crc16_ccitt_compute(TICOS_CRC16_CCITT_INITIAL_VALUE, codeword1,
                                     sizeof(codeword1));
  LONGS_EQUAL(0x0, crc);

  const uint8_t codeword2[] = {0x43, 0x61, 0x74, 0x4D, 0x6F, 0x75, 0x73,
                               0x65, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34,
                               0x33, 0x32, 0x31, 0xE5, 0x56};
  crc = ticos_crc16_ccitt_compute(TICOS_CRC16_CCITT_INITIAL_VALUE, codeword2,
                                     sizeof(codeword2));
  LONGS_EQUAL(0x0, crc);
}

TEST(TcsCrc16CcittGroup, Test_TcsCrc16CcittGroupComputeTestPattern) {
  const char *test_pattern = "123456789";

  uint16_t crc = ticos_crc16_ccitt_compute(
      TICOS_CRC16_CCITT_INITIAL_VALUE, test_pattern, strlen(test_pattern));
  LONGS_EQUAL(0x31C3, crc);
}

TEST(TcsCrc16CcittGroup, Test_TcsCrc16CcittGroupComputeTestPatternOneByte) {
  const char *test_pattern = "A";

  uint16_t crc = ticos_crc16_ccitt_compute(
      TICOS_CRC16_CCITT_INITIAL_VALUE, test_pattern, strlen(test_pattern));
  LONGS_EQUAL(0x58E5, crc);
}


TEST(TcsCrc16CcittGroup, Test_TcsCrc16CcittGroupComputeTestPatternIncemental) {
  uint16_t crc = TICOS_CRC16_CCITT_INITIAL_VALUE;
  for (char i = '1'; i <= '9'; i++) {
    crc = ticos_crc16_ccitt_compute(crc, &i, sizeof(i));
  }
  LONGS_EQUAL(0x31C3, crc);
}
