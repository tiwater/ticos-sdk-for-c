//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! A collection of utilities that can be used to validate the platform port of
//! the ticos coredump storage API is working as expected.
//!
//! Example Usage:
//!
//! void validate_coredump_storage_implementation(void) {
//!    // exercise storage routines used during a fault
//!    __disable_irq();
//!    ticos_coredump_storage_debug_test_begin()
//!    __enabled_irq();
//!
//!   // analyze results from test and print results to console
//!   ticos_coredump_storage_debug_test_finish();
//! }

#include "ticos/panics/coredump.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ticos/core/compiler.h"
#include "ticos/core/debug_log.h"
#include "ticos/core/math.h"
#include "ticos/panics/platform/coredump.h"

typedef enum {
  kTicosCoredumpStorageTestOp_Prepare = 0,
  kTicosCoredumpStorageTestOp_Erase,
  kTicosCoredumpStorageTestOp_Write,
  kTicosCoredumpStorageTestOp_Clear,
  kTicosCoredumpStorageTestOp_GetInfo,
} eTicosCoredumpStorageTestOp;

typedef enum {
  kTicosCoredumpStorageResult_Success = 0,
  kTicosCoredumpStorageResult_PlatformApiFail,
  kTicosCoredumpStorageResult_ReadFailed,
  kTicosCoredumpStorageResult_CompareFailed,
} eTicosCoredumpStorageResult;

typedef struct {
  eTicosCoredumpStorageResult result;
  eTicosCoredumpStorageTestOp op;
  uint32_t offset;
  const uint8_t *expected_buf;
  uint32_t size;
} sTicosCoredumpStorageTestResult;

static sTicosCoredumpStorageTestResult s_test_result;
static uint8_t s_read_buf[16];

static void prv_record_failure(
    eTicosCoredumpStorageTestOp op, eTicosCoredumpStorageResult result,
    uint32_t offset, uint32_t size) {
  s_test_result = (sTicosCoredumpStorageTestResult) {
    .op = op,
    .result = result,
    .offset = offset,
    .size = size,
  };
}

//! Helper to scrub the read buffer with a pattern not used by the test
//! This way even if a call to ticos_platform_coredump_storage_read()
//! returns true but does not populate the buffer we will not have a match
static void prv_scrub_read_buf(void) {
  const uint8_t unused_pattern = 0xef;
  memset(s_read_buf, unused_pattern, sizeof(s_read_buf));
}

static bool prv_verify_erased(uint8_t byte) {
  // NB: Depending on storage topology, pattern can differ
  //   0x00 if coredump storage is in RAM
  //   0xFF if coredump storage is some type of flash (i.e NOR, EMMC, etc)
  return ((byte == 0x00) || (byte == 0xff));
}

bool ticos_coredump_storage_debug_test_begin(void) {
  sTcsCoredumpStorageInfo info = { 0 };
  ticos_platform_coredump_storage_get_info(&info);
  if (info.size == 0) {
    prv_record_failure(kTicosCoredumpStorageTestOp_GetInfo,
                       kTicosCoredumpStorageResult_PlatformApiFail,
                       0, info.size);
    return false;
  }

  // On some ports there maybe some extra setup that needs to occur
  // before we can safely use the backing store without interrupts
  // enabled. Call this setup function now.
  if (!ticos_platform_coredump_save_begin()) {
    prv_record_failure(kTicosCoredumpStorageTestOp_Prepare,
                       kTicosCoredumpStorageResult_PlatformApiFail,
                       0, info.size);
    return false;
  }

  //
  // Confirm we can erase the storage region
  //

  if (!ticos_platform_coredump_storage_erase(0, info.size)) {
    prv_record_failure(kTicosCoredumpStorageTestOp_Erase,
                       kTicosCoredumpStorageResult_PlatformApiFail,
                       0, info.size);
    return false;
  }

  for (size_t i = 0; i < info.size; i+= sizeof(s_read_buf)) {
    prv_scrub_read_buf();

    const size_t bytes_to_read = TICOS_MIN(sizeof(s_read_buf), info.size - i);
    if (!ticos_platform_coredump_storage_read(i, s_read_buf, bytes_to_read)) {
      prv_record_failure(kTicosCoredumpStorageTestOp_Erase,
                         kTicosCoredumpStorageResult_ReadFailed, i, bytes_to_read);
      return false;
    }

    for (size_t j = 0; j <  bytes_to_read; j++) {
      if (!prv_verify_erased(s_read_buf[j])) {
        prv_record_failure(kTicosCoredumpStorageTestOp_Erase,
                           kTicosCoredumpStorageResult_CompareFailed, j + i, 1);
        return false;
      }
    }
  }

  //
  // Confirm we can write to storage by alternating writes of a 12 byte & 7 byte pattern.
  // This way we can verify that writes starting at different offsets are working
  //
  // The data for ticos coredumps is always written sequentially with the exception of
  // the 12 byte header which is written last. We will simulate that behavior here.
  //

  static const uint8_t pattern1[] = {
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab };
  TICOS_STATIC_ASSERT(sizeof(pattern1) < sizeof(s_read_buf), "pattern1 is too long");

  static const uint8_t pattern2[] = { 0x5f, 0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x59};
  TICOS_STATIC_ASSERT(sizeof(pattern2) < sizeof(s_read_buf), "pattern2 is too long");

  struct {
    const uint8_t *pattern;
    size_t len;
  } patterns[] = {
    {
      .pattern = pattern1,
      .len = sizeof(pattern1)
    },
    {
      .pattern = pattern2,
      .len = sizeof(pattern2)
    },
  };

  #define TICOS_COREDUMP_STORAGE_HEADER_LEN 12
  TICOS_STATIC_ASSERT(sizeof(pattern1) == TICOS_COREDUMP_STORAGE_HEADER_LEN,
                         "pattern1 length must match coredump header size");

  // Skip the "header" and begin writing alternating patterns to various offsets
  size_t offset = sizeof(pattern1);
  size_t num_writes = 1;
  while (offset < info.size) {
    const size_t pattern_idx = num_writes % TICOS_ARRAY_SIZE(patterns);
    num_writes++;

    const uint8_t *pattern = patterns[pattern_idx].pattern;
    size_t pattern_len = patterns[pattern_idx].len;
    pattern_len = TICOS_MIN(pattern_len, info.size - offset);

    if (!ticos_platform_coredump_storage_write(offset, pattern, pattern_len)) {
      prv_record_failure(kTicosCoredumpStorageTestOp_Write,
                         kTicosCoredumpStorageResult_PlatformApiFail, offset, pattern_len);
      return false;
    }
    offset += pattern_len;
  }

  // now simulate writing a coredump header
  if (!ticos_platform_coredump_storage_write(0, pattern1, sizeof(pattern1))) {
    prv_record_failure(kTicosCoredumpStorageTestOp_Write,
                       kTicosCoredumpStorageResult_PlatformApiFail, offset, sizeof(pattern1));
    return false;
  }

  // now read back what we wrote and confirm it matches the expected pattern sequence
  offset = 0;
  num_writes = 0;
  while (offset < info.size) {
    const size_t pattern_idx = num_writes % TICOS_ARRAY_SIZE(patterns);
    num_writes++;

    const uint8_t *pattern = patterns[pattern_idx].pattern;
    size_t pattern_len = patterns[pattern_idx].len;
    pattern_len = TICOS_MIN(pattern_len, info.size - offset);

    prv_scrub_read_buf();

    if (!ticos_platform_coredump_storage_read(offset, s_read_buf, pattern_len)) {
      prv_record_failure(kTicosCoredumpStorageTestOp_Write,
                         kTicosCoredumpStorageResult_ReadFailed, offset, pattern_len);
      return false;
    }

    if (memcmp(s_read_buf, pattern, pattern_len) != 0) {
      prv_record_failure(kTicosCoredumpStorageTestOp_Write,
                         kTicosCoredumpStorageResult_CompareFailed, offset, pattern_len);
      s_test_result.expected_buf = pattern;
      return false;
    }

    offset += pattern_len;
  }

  s_test_result = (sTicosCoredumpStorageTestResult) {
    .result = kTicosCoredumpStorageResult_Success,
  };
  return true;
}

static bool prv_verify_coredump_clear_operation(void) {
  ticos_platform_coredump_storage_clear();

  const size_t min_clear_size = 1;
  prv_scrub_read_buf();

  // NB: ticos_coredump_read() instead of ticos_platform_coredump_read() here because that's
  // the routine we use when the system is running (in case that mode needs locking)
  if (!ticos_coredump_read(0, s_read_buf, min_clear_size)) {
    prv_record_failure(kTicosCoredumpStorageTestOp_Clear,
                       kTicosCoredumpStorageResult_ReadFailed, 0, min_clear_size);
    return false;
  }

  if (!prv_verify_erased(s_read_buf[0])) {
    prv_record_failure(kTicosCoredumpStorageTestOp_Clear,
                       kTicosCoredumpStorageResult_CompareFailed, 0, 1);
    return false;
  }

  return true;
}

static void prv_hexdump(const char *prefix, const uint8_t *buf, size_t buf_len) {
  #define MAX_BUF_LEN (sizeof(s_read_buf) * 2 + 1)
  char hex_buffer[MAX_BUF_LEN];
  for (uint32_t j = 0; j < buf_len; ++j) {
    sprintf(&hex_buffer[j * 2], "%02x", buf[j]);
  }
  // make sure buffer is NUL terminated even if buf_len = 0
  hex_buffer[buf_len * 2] = '\0';
  TICOS_LOG_INFO("%s: %s", prefix, hex_buffer);
}

bool ticos_coredump_storage_debug_test_finish(void) {
  if ((s_test_result.result == kTicosCoredumpStorageResult_Success)
      && prv_verify_coredump_clear_operation()) {
    TICOS_LOG_INFO("Coredump Storage Verification Passed");
    return true;
  }

  TICOS_LOG_INFO("Coredump Storage Verification Failed");

  const char *op_suffix;
  switch (s_test_result.op) {
    case kTicosCoredumpStorageTestOp_Prepare:
      op_suffix = "prepare";
      break;

    case kTicosCoredumpStorageTestOp_Erase:
      op_suffix = "erase";
      break;

    case kTicosCoredumpStorageTestOp_Write:
      op_suffix = "write";
      break;

    case kTicosCoredumpStorageTestOp_Clear:
      op_suffix = "clear";
      break;

    case kTicosCoredumpStorageTestOp_GetInfo:
      op_suffix = "get info";
      break;

    default:
      op_suffix = "unknown";
      break;
  }

  const char *reason_str = "";
  switch (s_test_result.result) {
    case kTicosCoredumpStorageResult_Success:
      break;

    case kTicosCoredumpStorageResult_PlatformApiFail:
      reason_str = "Api call failed during";
      break;

    case kTicosCoredumpStorageResult_ReadFailed:
      reason_str = "Call to ticos_platform_coredump_storage_read() failed during";
      break;

    case kTicosCoredumpStorageResult_CompareFailed:
      reason_str = "Read pattern mismatch during";
      break;

    default:
      reason_str = "Unknown";
      break;
  }

  TICOS_LOG_INFO("%s ticos_platform_coredump_storage_%s() test", reason_str, op_suffix);
  TICOS_LOG_INFO("Storage offset: 0x%08"PRIx32", %s size: %d", s_test_result.offset, op_suffix,
                    (int)s_test_result.size);

  if (s_test_result.result == kTicosCoredumpStorageResult_CompareFailed) {
    if (s_test_result.expected_buf != NULL) {
      prv_hexdump("Expected", s_test_result.expected_buf, s_test_result.size);
    } else if (s_test_result.op !=  kTicosCoredumpStorageTestOp_Write) {
      TICOS_LOG_INFO("expected erase pattern is 0xff or 0x00");
    }

    prv_hexdump("Actual  ", s_read_buf, s_test_result.size);
  }

  return false;
}
