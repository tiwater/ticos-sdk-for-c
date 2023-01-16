set(TICOS_SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)

list(APPEND TICOS_COMPONENTS core util panics demo http metrics)
include(${TICOS_SDK_ROOT}/cmake/Ticos.cmake)
ticos_library(${TICOS_SDK_ROOT} TICOS_COMPONENTS
  TICOS_COMPONENTS_SRCS TICOS_COMPONENTS_INC_FOLDERS ARCH_XTENSA)

# v4.0 and greater expose the IDF_VERSION in cmake. If we can't find
# it, we assume the end user is on v3.x

include($ENV{IDF_PATH}/tools/cmake/version.cmake OPTIONAL)

# Version 3.3.5 and later provide IDF_VERSION_MAJOR.
if(DEFINED IDF_VERSION_MAJOR)
  if(IDF_VERSION_MAJOR VERSION_EQUAL 5)
    set(TICOS_ESP_IDF_PORT v5.x)
  elseif(IDF_VERSION_MAJOR VERSION_EQUAL 4)
    set(TICOS_ESP_IDF_PORT v4.x)
  elseif(IDF_VERSION_MAJOR VERSION_EQUAL 3)
    set(TICOS_ESP_IDF_PORT v3.x)
  else()
    message(FATAL_ERROR "IDF_VERSION_MAJOR=${IDF_VERSION_MAJOR} provided is not currenly supported")
  endif()
else()
  message(STATUS "No IDF_VERSION_MAJOR provided, using esp-idf v3.x Ticos port")
  set(TICOS_ESP_IDF_PORT v3.x)
endif()

# esp-idf version specific porting files
list(APPEND TICOS_COMPONENTS_SRCS
  ${TICOS_ESP_IDF_PORT}/ticos_esp_spi_flash.c
)

include(${CMAKE_CURRENT_LIST_DIR}/${TICOS_ESP_IDF_PORT}/Ticos-esp-idf-compat.cmake)

# This directory holds ports that are common across v3.x and v4.x esp-idf releases
set(TICOS_ESP_IDF_PORT_COMMON common)
list(APPEND TICOS_COMPONENTS_SRCS
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_fault_handler.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_core.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_coredump.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_debug_log.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_demo_cli_cmds.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_http_client.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_http_client_buffer.c
    ${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_metrics.c
		${TICOS_ESP_IDF_PORT_COMMON}/ticos_platform_dump_flash.c
)

list(APPEND TICOS_COMPONENTS_INC_FOLDERS
   include
   include/${TICOS_ESP_IDF_PORT}
   $ENV{TICOS_PLATFORM_EXTRA_INCLUDES}
   $ENV{IDF_PATH}/components/espcoredump/include_core_dump
)

if(CONFIG_IDF_TARGET_ARCH_XTENSA)
  list(APPEND TICOS_COMPONENTS_INC_FOLDERS "$ENV{IDF_PATH}/components/espcoredump/include_core_dump/port/xtensa")
elseif(CONFIG_IDF_TARGET_ARCH_RISCV)
  list(APPEND TICOS_COMPONENTS_INC_FOLDERS "$ENV{IDF_PATH}/components/espcoredump/include_core_dump/port/riscv")
endif()

# Register Ticos SDK Component
set(COMPONENT_SRCS ${TICOS_COMPONENTS_SRCS})
set(COMPONENT_ADD_INCLUDEDIRS ${TICOS_COMPONENTS_INC_FOLDERS})
list(APPEND COMPONENT_REQUIRES
    ${TICOS_ESP_IDF_VERSION_SPECIFIC_REQUIRES}
    $ENV{TICOS_PLATFORM_PORT_COMPONENTS}
    freertos
    heap
    log
    soc
    spi_flash
    console
)

if($ENV{TICOS_ESP_HTTP_CLIENT_ENABLE})
list(APPEND COMPONENT_REQUIRES esp_http_client)
endif()
register_component()

# sdk_overrides/esp_http_client.c needs the (private) esp_http header files:
get_filename_component(this_directory_name . ABSOLUTE DIRECTORY)
get_filename_component(this_directory_name ${this_directory_name} NAME)
tcs_esp32_component_get_target(this_component ${this_directory_name})

component_compile_options(-DTICOS_ESP_HTTP_CLIENT_ENABLE=$ENV{TICOS_ESP_HTTP_CLIENT_ENABLE})

# We will intercept the panic handlers enabled by CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH
# and run the Ticos Fault Handler instead.
#
# Benefits here are:
#   FreeRTOS task list is walked server side instead of on device (so you can get crash data even if the lists are corrupted)
#   Much more flexibility in debug information collected (e.g. all RAM, just the current stack trace, select stacks and variables)
#   Data can be posted directly from device to Ticos cloud for deduplication and analysis
target_link_libraries(${this_component} INTERFACE "-Wl,--wrap=esp_core_dump_to_flash -Wl,--wrap=esp_core_dump_init -Wl,--wrap=esp_core_dump_image_get -Wl,--wrap=esp_core_dump_write")

# Include a linker script fragment to support compact logs, when enabled
get_filename_component(compact_log_linker_script ${TICOS_ESP_IDF_PORT_COMMON}/ticos_compact_log.ld ABSOLUTE)
target_link_libraries(
    ${this_component}
    INTERFACE
    -T ${compact_log_linker_script}
)
