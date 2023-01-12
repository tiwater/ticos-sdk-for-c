SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/metrics/src/ticos_metrics_serializer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_serializer_helper.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_minimal_cbor.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_build_id.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_event_storage.cpp \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_time.c \
  $(Tcs_TEST_STUB_DIR)/stub_ticos_log_save.c \

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_metrics_serializer.cpp \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_get_device_info.c \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
