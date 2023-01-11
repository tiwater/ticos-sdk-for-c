SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_log.c \
  $(MFLT_COMPONENTS_DIR)/core/src/ticos_log_data_source.c \
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

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_ticos_log_data_source.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
