SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_custom_data_recording.c \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_circular_buffer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_crc16_ccitt.c \
  $(MFLT_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_platform_time.c \
  $(MFLT_TEST_FAKE_DIR)/fake_ticos_sdk_assert.c \
  $(MFLT_TEST_STUB_DIR)/stub_ticos_log_save.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_custom_data_recording.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_CDR_ENABLE=1

include $(CPPUTEST_MAKFILE_INFRA)
