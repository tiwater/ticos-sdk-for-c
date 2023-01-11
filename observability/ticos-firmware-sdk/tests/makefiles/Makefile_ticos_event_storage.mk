SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_event_storage.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_circular_buffer.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_sdk_assert.c \
  $(MFLT_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_event_storage.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_TEST_PERSISTENT_EVENT_STORAGE_DISABLE=0
CPPUTEST_CPPFLAGS += -DTICOS_EVENT_STORAGE_READ_BATCHING_ENABLED=0

include $(CPPUTEST_MAKFILE_INFRA)
