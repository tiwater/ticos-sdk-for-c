#include "mock_ticos_coredump.h"

#include "CppUTestExt/MockSupport.h"

extern "C" {
  #include <stdbool.h>
  #include <string.h>

  #include "ticos/core/data_packetizer_source.h"
  #include "ticos/panics/coredump.h"
  #include "ticos/panics/platform/coredump.h"
}

uint32_t g_mock_ticos_coredump_total_size = COREDUMP_TOTAL_SIZE;

bool ticos_platform_coredump_storage_read(uint32_t offset, void *data,
                                             size_t read_len) {
  const bool rv = mock().actualCall(__func__)
                        .withUnsignedIntParameter("offset", offset)
                        .withUnsignedIntParameter("read_len", (unsigned int)read_len)
                        .returnBoolValueOrDefault(true);
  if (rv) {
    memset(data, 0xff, read_len);
  }
  return rv;
}

bool ticos_coredump_has_valid_coredump(size_t *total_size_out) {
  const bool rv = mock().actualCall(__func__)
                        .withOutputParameter("total_size_out", total_size_out)
                        .returnBoolValueOrDefault(true);
  if (rv && total_size_out) {
    *total_size_out = g_mock_ticos_coredump_total_size;
  }
  return rv;
}

void ticos_platform_coredump_storage_clear(void) {
  mock().actualCall(__func__);
}

const sTicosDataSourceImpl g_ticos_coredump_data_source = {
  .has_more_msgs_cb = ticos_coredump_has_valid_coredump,
  .read_msg_cb = ticos_platform_coredump_storage_read,
  .mark_msg_read_cb = ticos_platform_coredump_storage_clear,
};
