//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//! Ram backed ticos storage implementation

#include "fake_ticos_platform_coredump_storage.h"

#include <assert.h>
#include <string.h>

#include "ticos/panics/platform/coredump.h"

typedef struct FakeTcsStorage {
  uint8_t *buf;
  size_t size;
  size_t sector_size;
} sFakeTcsStorage;

static sFakeTcsStorage s_fake_tcs_storage_ctx;

void ticos_platform_coredump_storage_get_info(sTcsCoredumpStorageInfo *info) {
  *info = (sTcsCoredumpStorageInfo) {
    .size = s_fake_tcs_storage_ctx.size,
    .sector_size = s_fake_tcs_storage_ctx.sector_size,
  };
}


void fake_ticos_platform_coredump_storage_setup(
  void *storage_buf, size_t storage_size, size_t sector_size) {
  s_fake_tcs_storage_ctx = (sFakeTcsStorage) {
    .buf = storage_buf,
    .size =  storage_size,
    .sector_size = sector_size,
  };
}

bool fake_ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                                  size_t read_len) {
  assert(s_fake_tcs_storage_ctx.buf != NULL);
  if ((offset + read_len) > s_fake_tcs_storage_ctx.size) {
    return false;
  }


  uint8_t *read_ptr = &s_fake_tcs_storage_ctx.buf[offset];
  memcpy(data, read_ptr, read_len);
  return true;
}

bool ticos_platform_coredump_storage_write(uint32_t offset, const void *data,
                                              size_t data_len) {
  assert(s_fake_tcs_storage_ctx.buf != NULL);
  if ((offset + data_len) > s_fake_tcs_storage_ctx.size) {
    return false;
  }

  uint8_t *write_ptr = &s_fake_tcs_storage_ctx.buf[offset];
  memcpy(write_ptr, data, data_len);
  return true;
}

bool ticos_platform_coredump_storage_erase(uint32_t offset, size_t erase_size) {
  const size_t sector_size = s_fake_tcs_storage_ctx.sector_size;
  assert((erase_size % sector_size) == 0);
  assert((offset % sector_size) == 0);

  for (size_t i = offset; i < erase_size; i += sector_size) {
    uint8_t erase_pattern[sector_size];
    memset(erase_pattern, 0xff, sizeof(erase_pattern));
    if (!ticos_platform_coredump_storage_write(
            i + offset, erase_pattern, sizeof(erase_pattern))) {
      return false;
    }
  }

  return true;
}

void ticos_platform_coredump_storage_clear(void) {
  uint8_t clear_byte = 0x0;
  bool success = ticos_platform_coredump_storage_write(0, &clear_byte, sizeof(clear_byte));
  assert(success);
}
