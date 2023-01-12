SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_minimal_cbor.c

include $(CPPUTEST_MAKFILE_INFRA)
