# Arguments that a typical make setup should already have defined
BUILDDIR := ./build
CFLAGS := -mcpu=cortex-m7 -mthumb
CC := arm-none-eabi-gcc

#
# Directory locations which need to be specified prior to including the
# example STM32 makefile
#

EMBEDDED_Tcs_SDK_ROOT := ../../..
STM32_CUBE_DIR := ../cube/fw_h7
include ticos_sdk.mk

# Simple 'all' and 'clean' rules. These should already exist in a real
# makefile target

all: $(EMBEDDED_Tcs_SDK_OBJS)
	@echo "Success"

clean:
	rm -rf $(BUILDDIR)

# An example set of rules for emitting the code needed for an stm32 ticos
# integration out to a build directory

VPATH += $(sort $(dir $(EMBEDDED_Tcs_SDK_SRCS)))

$(EMBEDDED_Tcs_SDK_OBJS): $(EMBEDDED_Tcs_OBJ_DIR)/%.o: ./%.c $(MAKEFILE_LIST)
	@echo Compiling $(<F)
	@$(CC) -c $(CFLAGS) -I. -DSTM32H743xx -DUSE_RTOS=0 -Wno-unused-parameter $(EMBEDDED_Tcs_SDK_INC) $< -o $@

$(EMBEDDED_Tcs_SDK_OBJS): | $(EMBEDDED_Tcs_OBJ_DIR)

$(EMBEDDED_Tcs_OBJ_DIR):
	@mkdir -p $(EMBEDDED_Tcs_OBJ_DIR)
