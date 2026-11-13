#!/bin/bash
# -------------------------------------------------------------------------
# This file is part of the MindStudio project.
# Copyright (c) 2026 Huawei Technologies Co.,Ltd.
#
# MindStudio is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
# -------------------------------------------------------------------------
set -euo pipefail

CUR_DIR=$(dirname "$(readlink -f "$0")")
TOP_DIR=$(readlink -f "${CUR_DIR}/..")

# Keep the coverage gate in memory; the default mode still generates reports.
if [ "${1:-}" = "--check" ]; then
    cd "${TOP_DIR}"
    # GCC may emit negative branch counters; warn without discarding line counts.
    "${2:-python3}" -m gcovr --root "${TOP_DIR}" --filter 'csrc/' \
        --object-directory "${TOP_DIR}/build_ut" \
        --gcov-ignore-parse-errors negative_hits.warn_once_per_file \
        --json-summary -j 8 "${TOP_DIR}/build_ut/test/ut/CMakeFiles/mssanitizer_test.dir"
    exit 0
fi

echo "***************Generate Coverage*****************"

COVERAGE_PATH=${TOP_DIR}/build_ut/coverage

if [ -d "${COVERAGE_PATH}" ]; then
    rm -rf "${COVERAGE_PATH}"
fi
mkdir -p "${COVERAGE_PATH}"
cd "${TOP_DIR}/build_ut"

LCOV_RC="--rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1"

lcov -c -d test/ut/CMakeFiles/mssanitizer_test.dir/ -o "${COVERAGE_PATH}/mssanitizer_test.info" -b "${COVERAGE_PATH}" $LCOV_RC

lcov -r "${COVERAGE_PATH}/mssanitizer_test.info" '*platform*' -o "${COVERAGE_PATH}/mssanitizer_test.info" $LCOV_RC
lcov -r "${COVERAGE_PATH}/mssanitizer_test.info" '*opensource*' -o "${COVERAGE_PATH}/mssanitizer_test.info" $LCOV_RC
lcov -r "${COVERAGE_PATH}/mssanitizer_test.info" '*test*' -o "${COVERAGE_PATH}/mssanitizer_test.info" $LCOV_RC
lcov -r "${COVERAGE_PATH}/mssanitizer_test.info" '*c++*' -o "${COVERAGE_PATH}/mssanitizer_test.info" $LCOV_RC
lcov -r "${COVERAGE_PATH}/mssanitizer_test.info" '/usr/include/*' -o "${COVERAGE_PATH}/mssanitizer_test.info" $LCOV_RC

genhtml "${COVERAGE_PATH}/mssanitizer_test.info" -o "${COVERAGE_PATH}/report" --branch-coverage

for test_detail in "${TOP_DIR}/build_ut/test_detail.xml" "${TOP_DIR}/build_ut/test/ut/test_detail.xml"; do
    if [ -f "${test_detail}" ]; then
        cp "${test_detail}" "${COVERAGE_PATH}/report/"
        break
    fi
done

cd "${COVERAGE_PATH}"
tar -zcvf report.tar.gz ./report
