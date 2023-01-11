zephyr_library_sources(ticos_platform_debug_log.c)
zephyr_library_sources(ticos_platform_core.c)
zephyr_library_sources(ticos_zephyr_ram_regions.c)
zephyr_library_sources(ticos_platform_lock.c)
zephyr_library_sources(ticos_platform_coredump_regions.c)

zephyr_library_sources_ifdef(CONFIG_TICOS_HTTP_ENABLE ticos_platform_http.c)
zephyr_library_sources_ifdef(CONFIG_TICOS_SHELL ticos_demo_cli.c)
zephyr_library_sources_ifdef(CONFIG_TICOS_RAM_BACKED_COREDUMP ticos_platform_ram_backed_coredump.c)
zephyr_library_sources_ifdef(CONFIG_TICOS_ROOT_CERT_STORAGE_TLS_CREDENTIAL_STORAGE ticos_tls_root_cert_storage.c)

if (NOT CONFIG_TICOS_METRICS_TIMER_CUSTOM)
  zephyr_library_sources(ticos_platform_metrics.c)
endif()

if (NOT CONFIG_TICOS_SOFTWARE_WATCHDOG_CUSTOM)
  zephyr_library_sources(ticos_software_watchdog.c)
endif()

if (CONFIG_TICOS_CACHE_FAULT_REGS)
  # We need to wrap z_arm_fault() so we can get a copy
  # of the fault registers before Zephyr consumes and
  # clears the CFSR. See ports/zephyr/common/ticos_platform_core.c.
  zephyr_ld_options(-Wl,--wrap=z_arm_fault)
endif()

# Allow users to Kconfig ticos logging as a backend as needed. See
# ticos/sdk/embedded/ports/zephyr/Kconfig and adjust TICOS_LOGGING_XXX.
if (CONFIG_TICOS_LOGGING_ENABLE)
  # Version 2 logging was first introduced in
  #  https://github.com/zephyrproject-rtos/zephyr/commit/f6a40ae183ed0781ffcdeac4ba9263116247a750
  #  and launched as part of the Zephyr v2.6 release
  #    https://github.com/zephyrproject-rtos/zephyr/pull/31535
  # It fully replaced version 1 after the 3.1 release: https://github.com/zephyrproject-rtos/zephyr/issues/46500
  # and the option to enable logging v1 via CONFIG_LOG1 was entirely removed
  if(${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_GREATER_EQUAL "3.1.99" OR CONFIG_LOG2)
    zephyr_library_sources(ticos_logging.c)
  else()
    zephyr_library_sources(ticos_logging_legacy.c)
  endif()
endif()

if(${KERNEL_VERSION_MAJOR}.${KERNEL_VERSION_MINOR}.${KERNEL_PATCHLEVEL} VERSION_GREATER_EQUAL "3.1.99")
   zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr)
endif()

zephyr_library_sources_ifdef(CONFIG_TICOS_HTTP_PERIODIC_UPLOAD ticos_http_periodic_upload.c)

# Make sure RAM based storage regions are not initialized on boot
# by placing them in special linker sections
zephyr_linker_sources(NOINIT ticos-no-init.ld)

zephyr_linker_sources(SECTIONS ticos-build-id.ld)

# Override the default Zephyr setting which disables the GNU Build ID
#   https://github.com/zephyrproject-rtos/zephyr/blob/d7ee114106eab485688223d97a49813d33b4cf21/cmake/linker/ld/target_base.cmake#L16
zephyr_ld_options("-Wl,--build-id")

if(CONFIG_TICOS_HEAP_STATS AND CONFIG_HEAP_MEM_POOL_SIZE GREATER 0)
  zephyr_ld_options(-Wl,--wrap=k_malloc)
  zephyr_ld_options(-Wl,--wrap=k_free)
endif()

zephyr_include_directories(.)
