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
echo "***************Generate Coverage*****************"

if [ -d "./coverage" ]; then
    rm -rf ./coverage
fi
mkdir coverage

lcov -c -d ./build_ut/test/ut/CMakeFiles/mssanitizer_test.dir -o ./coverage/mssanitizer_test.info -b ./coverage --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1

lcov -r ./coverage/mssanitizer_test.info '*platform*' -o ./coverage/mssanitizer_test.info --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1
lcov -r ./coverage/mssanitizer_test.info '*opensource*' -o ./coverage/mssanitizer_test.info --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1
lcov -r ./coverage/mssanitizer_test.info '*test*' -o ./coverage/mssanitizer_test.info --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1
lcov -r ./coverage/mssanitizer_test.info '*c++*' -o ./coverage/mssanitizer_test.info --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1
lcov -r ./coverage/mssanitizer_test.info '/usr/include/*' -o ./coverage/mssanitizer_test.info --rc lcov_branch_coverage=1 --rc geninfo_no_exception_branch=1

genhtml ./coverage/mssanitizer_test.info -o ./coverage/report --branch-coverage

mv test_detail.xml coverage/report/

cd coverage
tar -zcvf report.tar.gz ./report
