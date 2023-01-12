SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_compact_log_c.c


MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_compact_log_macros.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_COMPACT_LOG_ENABLE=1

include $(CPPUTEST_MAKFILE_INFRA)
