#!/usr/bin/env bash
# ----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# ----------------------------------------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR="${SCRIPT_DIR}/build"
LOG_DIR="${1:-${SCRIPT_DIR}/test_output}"
ARCH="${CMAKE_ASC_ARCHITECTURES:-dav-2201}"
MSSANITIZER_BIN="${MSSANITIZER_BIN:-mssanitizer}"
MSTX_INCLUDE_DIR="${MSTX_INCLUDE_DIR:-}"

mkdir -p "${LOG_DIR}"
chmod 750 "${LOG_DIR}"
rm -rf "${BUILD_DIR}"

cmake_args=(-DCMAKE_ASC_ARCHITECTURES="${ARCH}")
if [[ -n "${MSTX_INCLUDE_DIR}" ]]; then
    cmake_args+=(-DMSTX_INCLUDE_DIR="${MSTX_INCLUDE_DIR}")
fi
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" "${cmake_args[@]}"
cmake --build "${BUILD_DIR}" -j

assert_hazard_types()
{
    local log_file=$1
    local hazard
    for hazard in WAW WAR RAW; do
        if ! grep -q "Potential ${hazard} hazard" "${log_file}"; then
            echo "[FAIL] ${log_file} 未检出 ${hazard} 竞争"
            return 1
        fi
    done
}

assert_clean_execution()
{
    local stdout_log=$1
    local unexpected
    for unexpected in \
        "exited abnormally" \
        "Parse record failed" \
        "Unknown RecordType" \
        "MSTX injection is not active" \
        "failed, ret="; do
        if grep -q "${unexpected}" "${stdout_log}"; then
            echo "[FAIL] ${stdout_log} 出现异常执行信息：${unexpected}"
            return 1
        fi
    done
}

assert_log_contains()
{
    local log_file=$1
    local pattern=$2
    local description=$3

    if ! grep -q "${pattern}" "${log_file}"; then
        echo "[FAIL] ${log_file} 未包含${description}：${pattern}"
        return 1
    fi
}

run_case()
{
    local mode=$1
    local sanitizer_log="${LOG_DIR}/${mode}.log"
    local stdout_log="${LOG_DIR}/${mode}.stdout.log"
    local -a options=(--tool=racecheck --log-file="${sanitizer_log}")
    if [[ "${mode}" == "cross-npu" ]]; then
        options+=(--check-cross-npu-races=yes)
    fi

    : >"${sanitizer_log}"
    "${MSSANITIZER_BIN}" "${options[@]}" "${BUILD_DIR}/demo" "${mode}" >"${stdout_log}" 2>&1
    assert_clean_execution "${stdout_log}"
    assert_hazard_types "${sanitizer_log}"

    if [[ "${mode}" == "cross-core" ]]; then
        assert_log_contains "${sanitizer_log}" "block 0" "预期的核标识"
        assert_log_contains "${sanitizer_log}" "block 1" "预期的核标识"
    elif [[ "${mode}" == "cross-npu" ]]; then
        assert_log_contains "${sanitizer_log}" "device 0" "预期的设备标识"
        assert_log_contains "${sanitizer_log}" "device 1" "预期的设备标识"
        assert_log_contains "${sanitizer_log}" "Start cross npu racecheck" "跨 NPU 场景启动标识"
        assert_log_contains "${sanitizer_log}" \
            "Cross npu racecheck finished. See all detected errors above" \
            "跨 NPU 场景完成标识"
        if grep -q "Cross npu racecheck finished. No error detected" "${sanitizer_log}"; then
            echo "[FAIL] ${sanitizer_log} 未检出卡间竞争"
            return 1
        fi
    fi

    echo "[PASS] ${mode}: WAW/WAR/RAW 检测结果符合预期"
}

run_case pipeline
run_case cross-core
run_case cross-npu

echo "[PASS] sample_racecheck_advanced 全部场景验证通过"
