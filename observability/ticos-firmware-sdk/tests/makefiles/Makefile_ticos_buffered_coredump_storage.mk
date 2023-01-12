SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_buffered_coredump_storage_impl.c

MOCK_AND_FAKE_SRC_FILES +=

TEST_SRC_FILES = \
  $(MOCK_AND_FAKE_SRC_FILES) \
  $(Tcs_TEST_SRC_DIR)/test_ticos_buffered_coredump_storage.cpp

include $(CPPUTEST_MAKFILE_INFRA)
