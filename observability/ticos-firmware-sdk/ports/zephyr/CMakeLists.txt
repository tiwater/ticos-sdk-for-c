if(CONFIG_TICOS)
  set(TICOS_SDK_ROOT ../..)

  if(NOT DEFINED TICOS_ZEPHYR_PORT_TARGET)
    # Pick up latest supported target by default.
    # Currently works for Zephyr >= 2.4
    set(TICOS_ZEPHYR_PORT_TARGET v2.4)
  endif()

  # Collect Ticos SDK dependencies
  list(APPEND TICOS_COMPONENTS core util panics demo)

  if(CONFIG_TICOS_HTTP_ENABLE)
    list(APPEND TICOS_COMPONENTS http)
  endif()

  if (CONFIG_TICOS_METRICS)
    list(APPEND TICOS_COMPONENTS metrics)
  endif()

  include(${TICOS_SDK_ROOT}/cmake/Ticos.cmake)
  ticos_library(${TICOS_SDK_ROOT} TICOS_COMPONENTS
    TICOS_COMPONENTS_SRCS TICOS_COMPONENTS_INC_FOLDERS)

  # Add Ticos SDK sources to ticos library
  zephyr_interface_library_named(ticos)
  zephyr_library()
  zephyr_library_sources(${TICOS_COMPONENTS_SRCS})
  zephyr_include_directories(
    ${TICOS_COMPONENTS_INC_FOLDERS}
    ${TICOS_SDK_ROOT}/ports/include
    ${TICOS_SDK_ROOT}/ports/zephyr/config
    include
  )

  # Be sure to use the Zephyr override config file if config'd in. This file
  # will include the base config file. zephyr_compile_definitions() use
  # INTERFACE as the scope so users will pickup this define as expected.
  zephyr_compile_definitions(TICOS_PLATFORM_CONFIG_FILE=\"ticos_zephyr_platform_config.h\")

  # We automatically collect some Zephyr kernel metrics from a custom Zephyr port def file. The
  # Zephyr port def file will pull in the user's file via include directive.
  zephyr_compile_definitions(TICOS_METRICS_USER_HEARTBEAT_DEFS_FILE=\"ticos_metrics_heartbeat_zephyr_port_config.def\")

  # Zephyr specific port def for trace errors
  zephyr_compile_definitions(TICOS_TRACE_REASON_USER_DEFS_FILE=\"ticos_trace_reason_zephyr_port_config.def\")

  # Pick up Zephyr specific port files
  add_subdirectory(${TICOS_ZEPHYR_PORT_TARGET})
  add_subdirectory(common)
  add_subdirectory_ifdef(CONFIG_TICOS_NRF_CONNECT_SDK ncs)

  # Link Ticos Library
  zephyr_library_link_libraries(ticos)
  target_link_libraries(ticos INTERFACE zephyr_interface)

  # The Zephyr includes moved from /include to /include/zephyr in Zephyr 3.2:
  # https://github.com/zephyrproject-rtos/zephyr/commit/53ef68d4598b2f9005c5da3fc0b860ca1999d350
  # Add the old path for backwards compatibility. Note that Zephyr itself
  # supports the Kconfig option 'CONFIG_LEGACY_INCLUDE_PATH' to enable the same
  # compatibility behavior, but it's marked as deprecated and to be removed in
  # the future, so just apply the compatibiltiy fix here.
  if(${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_GREATER_EQUAL "3.1.99")
    zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)
  endif()

endif()
