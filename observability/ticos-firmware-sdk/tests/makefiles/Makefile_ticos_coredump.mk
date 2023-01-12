SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/panics/src/ticos_coredump.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_coredump_storage.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_coredump.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
