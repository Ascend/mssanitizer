/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#ifndef PLUGIN_RECORD_MAIN_SCALAR_EMPTY_INSTRUCTIONS_H
#define PLUGIN_RECORD_MAIN_SCALAR_EMPTY_INSTRUCTIONS_H

#include <utility>
#include "kernel_pub_func.h"
#include "utils.h"
#include "recorder.h"

namespace Sanitizer {

template<RecordType recordType>
AICORE_FUNC_HEAD void MainScalarRecordEmptyEvent(EXTRA_PARAMS_DEC)
{
    if (MemInfoIsInvalid(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    MainScalarEmptyRecord record{};
    record.location.blockId = blockIdx;

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

}
#endif