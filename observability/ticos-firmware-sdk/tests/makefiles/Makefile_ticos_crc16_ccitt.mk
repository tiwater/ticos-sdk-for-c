SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_crc16_ccitt.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_crc16_ccitt.cpp

include $(CPPUTEST_MAKFILE_INFRA)
