SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_build_id.c \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_core_utils.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_MOCK_DIR)/mock_ticos_platform_debug_log.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_build_id.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_USE_GNU_BUILD_ID=0

include $(CPPUTEST_MAKFILE_INFRA)
