SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_task_watchdog.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(Tcs_TEST_MOCK_DIR)/mock_ticos_fault_handling.cpp

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_task_watchdog.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_TASK_WATCHDOG_ENABLE=1

# required for mock ticos_fault_handling_assert_extra
CPPUTEST_CPPFLAGS += -DTICOS_NORETURN=""

include $(CPPUTEST_MAKFILE_INFRA)
