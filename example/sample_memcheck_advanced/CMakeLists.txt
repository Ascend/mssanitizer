# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

set(CMAKE_ASC_ARCHITECTURES "dav-2201" CACHE STRING "NPU architecture: dav-2201, dav-3510")

find_package(ASC REQUIRED)

find_path(MSSANITIZER_ACL_INCLUDE_DIR
    NAMES acl.h
    HINTS "$ENV{ASCEND_HOME_PATH}/tools/mssanitizer/include/acl"
)
if(NOT MSSANITIZER_ACL_INCLUDE_DIR)
    message(FATAL_ERROR "mssanitizer acl.h not found; check ASCEND_HOME_PATH or set MSSANITIZER_ACL_INCLUDE_DIR")
endif()

find_library(MSSANITIZER_ACL_HOOK_LIBRARY
    NAMES ascend_acl_hook
    HINTS "$ENV{ASCEND_HOME_PATH}/tools/mssanitizer/lib64"
)
if(NOT MSSANITIZER_ACL_HOOK_LIBRARY)
    message(FATAL_ERROR "libascend_acl_hook.so not found; check ASCEND_HOME_PATH or set MSSANITIZER_ACL_HOOK_LIBRARY")
endif()

project(memcheck_advanced_sample LANGUAGES ASC CXX)

add_executable(demo
    memcheck_advanced.asc
)

target_include_directories(demo PRIVATE ${MSSANITIZER_ACL_INCLUDE_DIR})

target_compile_options(demo PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES}>
    -g
    --cce-enable-sanitizer
)

target_link_libraries(demo PRIVATE runtime dl ${MSSANITIZER_ACL_HOOK_LIBRARY})
