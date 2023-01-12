//! @file

#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <string.h>

#include "ticos/panics/coredump.h"
#include "ticos/panics/platform/coredump.h"

static bool s_inject_prepare_failure = false;
bool ticos_platform_coredump_save_begin(void) {
  if (s_inject_prepare_failure) {
    return false;
  }

  return true;
}

static uint8_t s_ram_backed_coredump_region[200];
#define COREDUMP_REGION_SIZE sizeof(s_ram_backed_coredump_region)

static bool s_inject_get_info_failure = false;
void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  if (s_inject_get_info_failure) {
    return; // fail to populate information
  }

  const size_t coredump_region_size = sizeof(s_ram_backed_coredump_region);

  *info = (sTcsCoredumpStorageInfo) {
    .size = coredump_region_size,
    .sector_size = coredump_region_size
  };
}

static int s_inject_read_failure_offset;

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  if ((offset + read_len) > COREDUMP_REGION_SIZE) {
    return false;
  }


  const uint8_t *read_ptr = &s_ram_backed_coredump_region[offset];

  if ((s_inject_read_failure_offset >= 0) &&
      (s_inject_read_failure_offset < (int)COREDUMP_REGION_SIZE)) {
    s_ram_backed_coredump_region[s_inject_read_failure_offset] = 0xff;
  }

  memcpy(data, read_ptr, read_len);

  // let's have most of the failures be fake successes and for one case
  // just return a driver read failure
  return (s_inject_read_failure_offset != 0);
}

bool ticos_coredump_read(uint32_t offset, void *data,
                            size_t read_len) {
  return ticos_platform_coredump_storage_read(offset, data, read_len);
}

typedef enum {
  kTicosEraseFailureMode_None = 0,
  kTicosEraseFailureMode_ClearFailure,
  kTicosEraseFailureMode_EraseDriverFailure,

  kTicosEraseFailureMode_NumModes,
} eTicosEraseFailureMode;

static eTicosEraseFailureMode s_inject_erase_failure;

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  if ((offset + erase_size) > COREDUMP_REGION_SIZE) {
    return false;
  }

  uint8_t *erase_ptr = &s_ram_backed_coredump_region[offset];
  memset(erase_ptr, 0x0, erase_size);

  switch ((int)s_inject_erase_failure) {
    case kTicosEraseFailureMode_ClearFailure:
      s_ram_backed_coredump_region[COREDUMP_REGION_SIZE / 2] = 0xa5;
      break;
    case kTicosEraseFailureMode_EraseDriverFailure:
      return false;
    default:
      break;
  }
  return true;
}

typedef enum {
  kTicosWriteFailureMode_None = 0,
  kTicosWriteFailureMode_WriteDriverFailure,
  kTicosWriteFailureMode_WordUnaligned,
  kTicosWriteFailureMode_PartialWriteFail,
  kTicosWriteFailureMode_LastByte,
  kTicosWriteFailureMode_WriteFailureAtOffset0,
  kTicosWriteFailureMode_ReadAfterWrite,

  kTicosWriteFailureMode_NumModes,
} eTicosWriteFailureMode;

static eTicosWriteFailureMode s_inject_write_failure;

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  if ((offset + data_len) > COREDUMP_REGION_SIZE) {
    return false;
  }

  switch ((int)s_inject_write_failure) {
    case kTicosWriteFailureMode_WriteDriverFailure:
      return false;

    case kTicosWriteFailureMode_WriteFailureAtOffset0:
      if (offset == 0) {
        return false;
      }
      break;

    case kTicosWriteFailureMode_WordUnaligned:
      if ((offset % 4) != 0) {
        return true; // silent fail
      }
      break;
    case kTicosWriteFailureMode_PartialWriteFail:
      // fail to write 1 byte
      data_len -= 1;
      break;
    case kTicosWriteFailureMode_LastByte:
      if ((offset + data_len) == COREDUMP_REGION_SIZE) {
        data_len -= 1;
      }
      break;
    case kTicosWriteFailureMode_ReadAfterWrite:
      s_inject_read_failure_offset = 0;
      break;
    default:
      break;
  }

  uint8_t *write_ptr = &s_ram_backed_coredump_region[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  const uint8_t clear_byte = 0x0;
  ticos_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
}

TEST_GROUP(TcsCoredumpStorageTestGroup) {
  void setup() {
    s_inject_write_failure = kTicosWriteFailureMode_None;
    s_inject_read_failure_offset = -1;
    s_inject_erase_failure = kTicosEraseFailureMode_None;
    s_inject_get_info_failure = false;
    s_inject_prepare_failure = false;
  }

  void teardown() {
  }
};

TEST(TcsCoredumpStorageTestGroup, Test_StorageImplementationGood) {
  bool success = ticos_coredump_storage_debug_test_begin();
  success &= ticos_coredump_storage_debug_test_finish();
  CHECK(success);
}

TEST(TcsCoredumpStorageTestGroup, Test_WriteFailureModes) {
  for (int i = 1; i < kTicosWriteFailureMode_NumModes; i++) {
    s_inject_write_failure = (eTicosWriteFailureMode)i;
    s_inject_read_failure_offset = -1;
    bool success = ticos_coredump_storage_debug_test_begin();
    success &= ticos_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(TcsCoredumpStorageTestGroup, Test_ReadCompareFailure) {
  for (size_t i = 0; i < COREDUMP_REGION_SIZE; i++) {
    s_inject_read_failure_offset = i;
    bool success = ticos_coredump_storage_debug_test_begin();
    success &= ticos_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(TcsCoredumpStorageTestGroup, Test_EraseFailure) {
  for (int i = 1; i < kTicosEraseFailureMode_NumModes; i++) {
    s_inject_erase_failure = (eTicosEraseFailureMode)i;
    bool success = ticos_coredump_storage_debug_test_begin();
    success &= ticos_coredump_storage_debug_test_finish();
    CHECK(!success);
  }
}

TEST(TcsCoredumpStorageTestGroup, Test_ClearFailureDueToWriteFailure) {
  bool success = ticos_coredump_storage_debug_test_begin();
  CHECK(success);

  s_inject_write_failure = (eTicosWriteFailureMode)kTicosWriteFailureMode_PartialWriteFail;
  success = ticos_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(TcsCoredumpStorageTestGroup, Test_ClearFailureDueToReadFailure) {
  bool success = ticos_coredump_storage_debug_test_begin();
  CHECK(success);

  s_inject_write_failure = (eTicosWriteFailureMode)kTicosWriteFailureMode_ReadAfterWrite;
  success = ticos_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(TcsCoredumpStorageTestGroup, Test_GetInfoFail) {
  s_inject_get_info_failure = true;
  bool success = ticos_coredump_storage_debug_test_begin();
  success &= ticos_coredump_storage_debug_test_finish();
  CHECK(!success);
}

TEST(TcsCoredumpStorageTestGroup, Test_PrepareFail) {
  s_inject_prepare_failure = true;
  bool success = ticos_coredump_storage_debug_test_begin();
  success &= ticos_coredump_storage_debug_test_finish();
  CHECK(!success);
}
