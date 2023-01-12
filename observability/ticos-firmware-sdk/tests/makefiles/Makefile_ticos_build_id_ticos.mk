SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_build_id.c \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_core_utils.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_MOCK_DIR)/mock_ticos_platform_debug_log.cpp \
  $(Tcs_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_build_id.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_USE_GNU_BUILD_ID=0

include $(CPPUTEST_MAKFILE_INFRA)
