SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_chunk_transport.c

MOCK_AND_FAKE_SRC_FILES += \
  $(Tcs_TEST_FAKE_DIR)/fake_ticos_platform_debug_log.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_varint.c \
  $(Tcs_COMPONENTS_DIR)/util/src/ticos_crc16_ccitt.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_chunk_transport.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
