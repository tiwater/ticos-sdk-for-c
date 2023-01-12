SRC_FILES = \
  $(Tcs_COMPONENTS_DIR)/core/src/ticos_ram_reboot_info_tracking.c

TEST_SRC_FILES = \
  $(Tcs_TEST_SRC_DIR)/test_ticos_ram_reboot_tracking.cpp

include $(CPPUTEST_MAKFILE_INFRA)
