SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_rle.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_varint.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_rle.cpp

include $(CPPUTEST_MAKFILE_INFRA)
