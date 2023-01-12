SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_base64.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_base64.cpp

include $(CPPUTEST_MAKFILE_INFRA)
