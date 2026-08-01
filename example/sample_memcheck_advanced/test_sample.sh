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
umask 027

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
BUILD_DIR="${SCRIPT_DIR}/build"
LOG_DIR="${1:-${SCRIPT_DIR}/test_output}"
ARCH="${CMAKE_ASC_ARCHITECTURES:-dav-2201}"
MSSANITIZER_BIN="${MSSANITIZER_BIN:-mssanitizer}"
MSSANITIZER_ACL_INCLUDE_DIR="${MSSANITIZER_ACL_INCLUDE_DIR:-}"
MSSANITIZER_ACL_HOOK_LIBRARY="${MSSANITIZER_ACL_HOOK_LIBRARY:-}"

mkdir -p "${LOG_DIR}"
chmod 750 "${LOG_DIR}"
rm -rf "${BUILD_DIR}"

cmake_args=(-DCMAKE_ASC_ARCHITECTURES="${ARCH}")
if [[ -n "${MSSANITIZER_ACL_INCLUDE_DIR}" ]]; then
    cmake_args+=(-DMSSANITIZER_ACL_INCLUDE_DIR="${MSSANITIZER_ACL_INCLUDE_DIR}")
fi
if [[ -n "${MSSANITIZER_ACL_HOOK_LIBRARY}" ]]; then
    cmake_args+=(-DMSSANITIZER_ACL_HOOK_LIBRARY="${MSSANITIZER_ACL_HOOK_LIBRARY}")
fi
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" "${cmake_args[@]}"
cmake --build "${BUILD_DIR}" -j

assert_clean_execution()
{
    local stdout_log=$1
    local unexpected
    for unexpected in \
        "exited abnormally" \
        "Parse record failed" \
        "Unknown RecordType" \
        "failed, ret="; do
        if grep -q "${unexpected}" "${stdout_log}"; then
            echo "[FAIL] ${stdout_log} 出现异常执行信息：${unexpected}"
            return 1
        fi
    done
}

assert_case_report()
{
    local mode=$1
    local log_file=$2
    local failed=0
    case "${mode}" in
        multi-core)
            grep -q "WARNING: out of bounds of size" "${log_file}" || failed=1
            grep -q "on GM when writing data" "${log_file}" || failed=1
            grep -q "multi_core_stomp" "${log_file}" || failed=1
            ;;
        l1-oob)
            grep -q "ERROR: illegal write of size" "${log_file}" || failed=1
            grep -q "on L1 in" "${log_file}" || failed=1
            grep -q "in block aic(" "${log_file}" || failed=1
            grep -q "l1_out_of_bounds" "${log_file}" || failed=1
            ;;
        l1-oob-read)
            grep -q "ERROR: illegal read of size" "${log_file}" || failed=1
            grep -q "on L1 in" "${log_file}" || failed=1
            grep -q "in block aic(" "${log_file}" || failed=1
            grep -q "l1_read_out_of_bounds" "${log_file}" || failed=1
            ;;
        l1-misaligned)
            grep -q "ERROR: misaligned access of size" "${log_file}" || failed=1
            grep -q "on L1 in" "${log_file}" || failed=1
            grep -q "in block aic(" "${log_file}" || failed=1
            grep -q "l1_misaligned_access" "${log_file}" || failed=1
            ;;
        misaligned)
            grep -q "ERROR: misaligned access of size" "${log_file}" || failed=1
            grep -q "on UB" "${log_file}" || failed=1
            grep -q "misaligned_access" "${log_file}" || failed=1
            ;;
        illegal-free)
            grep -q "ERROR: illegal free()" "${log_file}" || failed=1
            grep -q "on GM" "${log_file}" || failed=1
            grep -Eq "code in .+:[1-9][0-9]*" "${log_file}" || failed=1
            ;;
        leak)
            grep -q "ERROR: LeakCheck: detected memory leaks" "${log_file}" || failed=1
            grep -q "Direct leak of" "${log_file}" || failed=1
            grep -q "byte(s) leaked in" "${log_file}" || failed=1
            grep -Eq "allocated in .+:[1-9][0-9]*" "${log_file}" || failed=1
            ;;
        unused)
            grep -q "WARNING: Unused memory of" "${log_file}" || failed=1
            grep -q "byte(s) unused memory in" "${log_file}" || failed=1
            grep -q "on GM" "${log_file}" || failed=1
            ;;
    esac
    if [[ ${failed} -ne 0 ]]; then
        echo "[FAIL] ${mode}: 检测报告不符合预期，详见 ${log_file}"
        return 1
    fi
}

run_case()
{
    local mode=$1
    local sanitizer_log="${LOG_DIR}/${mode}.log"
    local stdout_log="${LOG_DIR}/${mode}.stdout.log"
    local -a options=(--tool=memcheck --log-file="${sanitizer_log}")
    if [[ "${mode}" == "illegal-free" ]]; then
        options+=(--check-cann-heap=yes)
    elif [[ "${mode}" == "leak" ]]; then
        options+=(--check-cann-heap=yes)
        options+=(--leak-check=yes)
    elif [[ "${mode}" == "unused" ]]; then
        options+=(--check-cann-heap=yes)
        options+=(--check-unused-memory=yes)
    fi

    : >"${sanitizer_log}"
    local sanitizer_status=0
    "${MSSANITIZER_BIN}" "${options[@]}" "${BUILD_DIR}/demo" "${mode}" >"${stdout_log}" 2>&1 || \
        sanitizer_status=$?
    if [[ ${sanitizer_status} -ne 0 ]]; then
        echo "[FAIL] ${mode}: mssanitizer 执行失败（退出码 ${sanitizer_status}），详见 ${stdout_log}"
        return "${sanitizer_status}"
    fi
    assert_clean_execution "${stdout_log}"
    assert_case_report "${mode}" "${sanitizer_log}"
    echo "[PASS] ${mode}: 检测结果符合预期"
}

run_case multi-core
run_case l1-oob
run_case l1-oob-read
run_case l1-misaligned
run_case misaligned
run_case illegal-free
run_case leak
run_case unused

echo "[PASS] sample_memcheck_advanced 全部场景验证通过"
