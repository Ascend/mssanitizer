#!/usr/bin/env bash
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
# =============================================================================
# clang-tidy wrapper — checks compile_commands.json before running clang-tidy
# =============================================================================
set -euo pipefail

BUILD_DIRS=("build")

found_dir=""
for dir in "${BUILD_DIRS[@]}"; do
    if [[ -f "${dir}/compile_commands.json" ]]; then
        found_dir="${dir}"
        break
    fi
done

# 编译库缺失时跳过 clang-tidy（例如 CI 的 pre-commit job 不执行构建），
# 避免因缺少 compile_commands.json 直接判失败。
if [[ -z "${found_dir}" ]]; then
    cat >&2 <<'EOF'
=======================================================================
  WARNING: compile_commands.json NOT FOUND, skip clang-tidy
-----------------------------------------------------------------------
  clang-tidy requires a compilation database to work. Generate it via:

    python3 build.py

  (This will create compile_commands.json under the build/ directory.)
  clang-tidy is skipped for this run.
=======================================================================
EOF
    exit 0
fi

exec clang-tidy "$@"
