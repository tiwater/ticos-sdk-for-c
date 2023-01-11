SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_sdk_assert.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(MFLT_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_sdk_assert.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
