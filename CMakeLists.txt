cmake_minimum_required(VERSION 3.13) # to support add_link_options

project(sanitizers VERSION 26.0.0)
set(CMAKE_SKIP_RPATH TRUE)

# 启用编译命令导出，用于IDE代码导航（跳转到定义）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty ABSOLUTE)
get_filename_component(JSON_INC ${THIRD_PARTY_DIR}/json/include/ ABSOLUTE)
get_filename_component(SECUREC_INC_DIR ${THIRD_PARTY_DIR}/securec/include ABSOLUTE)
get_filename_component(SANITIZER_INC_DIR csrc/include ABSOLUTE)
get_filename_component(SANITIZER_THIRDPARTY_INCLUDE csrc/include/thirdparty ABSOLUTE)
get_filename_component(MSOPSCOMMON_INC ${CMAKE_CURRENT_SOURCE_DIR}/msopscommon/csrc/kernel_injection/include/ ABSOLUTE)

# 检查是否可用 ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found, using native compiler.")
endif()

# 为了适用于工程本身构建，也适用于其它工程调用此工程，CMAKE_INSTALL_PREFIX的判断方式需要判断是否未设定或者系统默认路径
if (${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local" OR ${CMAKE_INSTALL_PREFIX} STREQUAL "/usr" OR NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${ROOT_DIR}/output CACHE STRING "install prefix" FORCE)
  message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}")
endif() 

option(BUILD_TESTS "build unit tests" OFF)

include(cmake/options.cmake)
include(cmake/module/securec.cmake)
add_subdirectory(csrc)

if (BUILD_TESTS)
  message(STATUS "Enable UT test build")
  enable_testing()
  add_subdirectory(thirdparty)
  add_subdirectory(test)
else()
  include(cmake/module/llvm.cmake)
  include(cmake/module/preparePackage.cmake)
endif()

option(BUILD_INJECTION "build function injection library" ON)
if(BUILD_INJECTION)
  set(BUILD_TESTS off)
  add_subdirectory(msopscommon EXCLUDE_FROM_ALL)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp
    COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_custom_target(injection_laucher ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tmp.cpp)
  add_dependencies(injection_laucher mssanitizer_injection kernel_launcher_bin)
  install(TARGETS mssanitizer_injection kernel_launcher_bin
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
          LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib64
  )
endif()

