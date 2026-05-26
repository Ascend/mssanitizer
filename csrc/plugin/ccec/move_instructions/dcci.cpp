/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2025 Huawei Technologies Co.,Ltd.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * ------------------------------------------------------------------------- */

#include "core/framework/record_defs.h"
#include "plugin/record_move_instructions.h"

using namespace Sanitizer;

// #1437
SANITIZER_REPORT(dcci, __gm__ void *dst, uint64_t entire) {
    RecordDcciEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), AddressSpace::GM,
        static_cast<DcciEntireType>(entire), DcciDstType::CACHE_LINE_ALL);
}

// #1438
SANITIZER_REPORT(dcci, __gm__ void *dst, uint64_t entire, uint64_t type) {
    RecordDcciEvent(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst), AddressSpace::GM,
        static_cast<DcciEntireType>(entire), static_cast<DcciDstType>(type));
}
