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

find_path(MSTX_INCLUDE_DIR
    NAMES mstx/ms_tools_ext.h
    HINTS
        "$ENV{ASCEND_HOME_PATH}/include"
        "$ENV{ASCEND_HOME_PATH}/aarch64-linux/include"
)
if(NOT MSTX_INCLUDE_DIR)
    message(FATAL_ERROR "mstx/ms_tools_ext.h not found; check ASCEND_HOME_PATH or set MSTX_INCLUDE_DIR")
endif()

project(racecheck_advanced_sample LANGUAGES ASC CXX)

add_executable(demo
    racecheck_advanced.asc
)

target_include_directories(demo PRIVATE ${MSTX_INCLUDE_DIR}/mstx)

target_compile_options(demo PRIVATE
    $<$<COMPILE_LANGUAGE:ASC>:--npu-arch=${CMAKE_ASC_ARCHITECTURES}>
    -g
    --cce-enable-sanitizer
)

target_link_libraries(demo PRIVATE runtime dl)
