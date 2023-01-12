SRC_FILES = \
  $(Tcs_PORTS_DIR)/nrf5_sdk/nrf5_coredump_regions.c \

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_port_nrf5_coredump_regions.cpp

CPPUTEST_CPPFLAGS += \
  -Werror=unused-macros \
  -DTICOS_PLATFORM_COREDUMP_CUSTOM_REGIONS=1 \

include $(CPPUTEST_MAKFILE_INFRA)
