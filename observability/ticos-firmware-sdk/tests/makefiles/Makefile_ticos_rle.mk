SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_rle.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_varint.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_rle.cpp

include $(CPPUTEST_MAKFILE_INFRA)
