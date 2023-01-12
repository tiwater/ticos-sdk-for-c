SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_batched_events.c

TEST_SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_sdk_assert.c \
  $(Tcs_TEST_SRC_DIR)/test_ticos_batched_events.cpp


include $(CPPUTEST_MAKFILE_INFRA)
