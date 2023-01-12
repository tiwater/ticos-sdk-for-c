SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_heap_stats.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_locking.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_heap_stats.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
