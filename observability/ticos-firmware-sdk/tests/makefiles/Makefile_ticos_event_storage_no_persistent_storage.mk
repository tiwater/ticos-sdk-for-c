SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_event_storage.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_circular_buffer.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_sdk_assert.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_event_storage.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_TEST_PERSISTENT_EVENT_STORAGE_DISABLE=1
CPPUTEST_CPPFLAGS += -DTICOS_EVENT_STORAGE_READ_BATCHING_ENABLED=0
CPPUTEST_CPPFLAGS += -DTICOS_EVENT_STORAGE_NV_SUPPORT_ENABLED=0

include $(CPPUTEST_MAKFILE_INFRA)
