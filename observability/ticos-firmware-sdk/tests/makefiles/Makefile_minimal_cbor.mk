SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_minimal_cbor.c

include $(CPPUTEST_MAKFILE_INFRA)
