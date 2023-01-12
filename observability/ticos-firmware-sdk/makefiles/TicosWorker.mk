# A convenience helper makefile that can be used to collect the sources and include
# flags needed for the Ticos SDK based on the components used
#
# USAGE
# If you are using a Make build system, to pick up the Ticos include paths & source
# files needed for a project, you can just add the following lines:
#
# TICOS_SDK_ROOT := <The path to the root of the ticos-firmware-sdk repo>
# TICOS_COMPONENTS := <The SDK components to be used, i.e "core panics util">
# include $(TICOS_SDK_ROOT)/makefiles/TicosWorker.mk
# <YOUR_SRC_FILES> += $(TICOS_COMPONENTS_SRCS)
# <YOUR_INCLUDE_PATHS> += $(TICOS_COMPONENTS_INC_FOLDERS)

# A utility to easily assert that a Makefile variable is defined and non-empty
#   Argument 1: The variable to check
#   Argument 2: The error message to display if the variable is not defined
ticos_assert_arg_defined = \
  $(if $(value $(strip $1)),,$(error Undefined $1:$2))

TICOS_VALID_COMPONENTS := core demo http panics util metrics

$(call ticos_assert_arg_defined,TICOS_COMPONENTS,\
  Must be set to one or more of "$(TICOS_VALID_COMPONENTS)")
$(call ticos_assert_arg_defined,TICOS_SDK_ROOT,\
  Must define the path to the root of the Ticos SDK)

TICOS_COMPONENTS_DIR := $(TICOS_SDK_ROOT)/components

TICOS_COMPONENTS_INC_FOLDERS := $(TICOS_COMPONENTS_DIR)/include

TICOS_COMPONENTS_SRCS = \
  $(foreach component, $(TICOS_COMPONENTS), \
    $(sort $(wildcard $(TICOS_COMPONENTS_DIR)/$(component)/src/*.c)) \
  )

ifneq ($(filter demo,$(TICOS_COMPONENTS)),)
# The demo component is enabled so let's pick up component specific cli commands
TICOS_COMPONENTS_SRCS += \
  $(foreach component, $(TICOS_COMPONENTS), \
    $(sort $(wildcard $(TICOS_COMPONENTS_DIR)/demo/src/$(component)/*.c)) \
  )
endif

TICOS_COMPONENTS_SRCS := \
  $(patsubst %ticos_fault_handling_xtensa.c, , $(TICOS_COMPONENTS_SRCS))
