NAME := TicosPlatformReferenceImpl

$(NAME)_SOURCES    := ticos_platform_coredump.c \
                      ticos_platform_crc32.c \
                      ticos_platform_debug_log.c \
                      ticos_platform_device_info.c \
                      ticos_platform_fault_handling_arm_gcc.c \
                      ticos_platform_http_client.c \
                      ticos_platform_impl.c \


$(NAME)_COMPONENTS := drivers/spi_flash \
                      libraries/ticos/core \


GLOBAL_LDFLAGS    += -Wl,-T $(SOURCE_ROOT)libraries/ticos/platform_reference_impl/ticos_platform_coredump.ld
GLOBAL_INCLUDES   += .

VALID_OSNS_COMBOS  := ThreadX-NetX_Duo FreeRTOS-LwIP
VALID_PLATFORMS := BCM943362WCD4 \
                   BCM943362WCD6 \
                   BCM943362WCD8 \
                   BCM943364WCD1 \
                   CYW94343WWCD1_EVB \
                   BCM943438WCD1 \
                   BCM94343WWCD2 \
                   CY8CKIT_062 \
                   NEB1DX* \
                   CYW9MCU7X9N364 \
                   CYW943907AEVAL1F \
                   CYW954907AEVAL1F \
                   CYW9WCD2REFAD2* \
                   CYW9WCD760PINSDAD2 \
                   CYW943455EVB* \
                   CYW943012EVB*
