# A convenience helper cmake function that can be used to collect the sources and include
# paths needed for the Ticos SDK based on the components used
#
# USAGE
# If you are using a Cmake build system, to pick up the Ticos include paths & source
# files needed for a project, you can just add the following lines:
#
# set(TICOS_SDK_ROOT <The path to the root of the ticos-firmware-sdk repo>)
# list(APPEND TICOS_COMPONENTS <The SDK components to be used, i.e "core util">)
# include(${TICOS_SDK_ROOT}/cmake/Ticos.cmake)
# ticos_library(${TICOS_SDK_ROOT} TICOS_COMPONENTS
#   TICOS_COMPONENTS_SRCS TICOS_COMPONENTS_INC_FOLDERS)
#
# NOTE: By default, the list of sources returned will be for ARM Cortex-M targets but ARCH_XTENSA can be
# passed as an optional final argument to return sources expected for ARCH_XTENSA architectures
#
# After invoking the function ${TICOS_COMPONENTS_SRCS} will contain the sources
# needed for the library and ${TICOS_COMPONENTS_INC_FOLDERS} will contain the include
# paths

# Explicitly enable IN_LIST use inside if()
#  https://cmake.org/cmake/help/v3.3/policy/CMP0057.html
cmake_policy(SET CMP0057 NEW)

function(ticos_library sdk_root components src_var_name inc_var_name)
  if(NOT IS_ABSOLUTE ${sdk_root})
    set(sdk_root "${CMAKE_CURRENT_SOURCE_DIR}/${sdk_root}")
  endif()

  if ("demo" IN_LIST ${components})
    # The demo component is enabled so let's pick up component specific cli commands
    # If the component is not enabled a weak implementation of the CLI command will get
    # picked up from "demo/src/ticos_demo_shell_commands.c"
    foreach(component IN LISTS ${components})
      file(GLOB TICOS_COMPONENT_DEMO_${component} ${sdk_root}/components/demo/src/${component}/*.c)
      list(APPEND SDK_SRC ${TICOS_COMPONENT_DEMO_${component}})
    endforeach()
  endif()


  list(APPEND SDK_INC
    ${sdk_root}/components/include
  )

  foreach(component IN LISTS ${components})
    file(GLOB TICOS_COMPONENT_${component} ${sdk_root}/components/${component}/src/*.c)
    list(APPEND SDK_SRC ${TICOS_COMPONENT_${component}})
  endforeach()

  set(arch ${ARGN})
  if(NOT arch)
    set(arch ARCH_ARM_CORTEX_M)
  endif()

  if(arch STREQUAL "ARCH_XTENSA")
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/core/src/arch_arm_cortex_m.c)
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/ticos_coredump_regions_armv7.c)
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/ticos_fault_handling_arm.c)
  elseif(arch STREQUAL "ARCH_ARM_CORTEX_M")
    list(REMOVE_ITEM SDK_SRC ${sdk_root}/components/panics/src/ticos_fault_handling_xtensa.c)
  else()
    message(FATAL_ERROR "Unsupported Arch: ${arch}")
  endif()
  set(${src_var_name} ${SDK_SRC} PARENT_SCOPE)
  set(${inc_var_name} ${SDK_INC} PARENT_SCOPE)
endfunction()
