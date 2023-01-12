SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_custom_data_recording.c \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_serializer_helper.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_circular_buffer.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_crc16_ccitt.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_build_id.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_get_device_info.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_locking.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_time.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_sdk_assert.c \
  $(Tcs_TEST_STUB_DIR)/stub_ticos_log_save.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_custom_data_recording.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTICOS_CDR_ENABLE=1

include $(CPPUTEST_MAKFILE_INFRA)
