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

#include "error_buffer.h"
#include "core/framework/utility/log.h"

namespace Sanitizer {

std::vector<ReducedErrorMsg> ErrorBuffer::GetBuffer()
{
    std::vector<ReducedErrorMsg> errorList(errorSet_.cbegin(), errorSet_.cend());
    std::sort(errorList.begin(), errorList.end(), [](ReducedErrorMsg const &lhs, ReducedErrorMsg const &rhs) {
        return lhs.errorMsg.auxData.serialNo < rhs.errorMsg.auxData.serialNo;
    });
    return errorList;
}


void ErrorBuffer::Add(const ErrorMsg &error)
{
    // 校验 ErrorMsg 是否有效。设备端/回放产生的损坏或未初始化 error（nBadBytes == 0）
    // 不应上报，避免 "illegal write of size 0" 之类的无意义误报。真实内存错误的 size 必 >= 1。
    if (error.auxData.nBadBytes == 0) {
        SAN_WARN_LOG(
            "Skip Corrupted Or Zeroed Error, type:%u, nBadBytes:0, addr:0x%lx, space:%u, serialNo:%lu, isSimt:%u",
            static_cast<uint32_t>(error.type), error.auxData.badAddr.addr, static_cast<uint32_t>(error.auxData.space),
            error.auxData.serialNo, static_cast<uint32_t>(error.auxData.isSimt));
        return;
    }
    ReducedErrorMsg reducedError{error, {}, {}, {}};
    if (error.auxData.blockType == BlockType::AIVEC) {
        reducedError.aivBlocks = {static_cast<uint64_t>(error.auxData.coreId)};
    } else if (error.auxData.blockType == BlockType::AICUBE) {
        reducedError.aicBlocks = {static_cast<uint64_t>(error.auxData.coreId)};
    } else {
        reducedError.aicoreBlocks = {static_cast<uint64_t>(error.auxData.coreId)};
    }
    auto it = errorSet_.find(reducedError);
    if (it == errorSet_.end()) {
        errorSet_.insert(reducedError);
    } else {
        if (error.auxData.blockType == BlockType::AIVEC) {
            it->aivBlocks.insert(error.auxData.coreId);
        } else if (error.auxData.blockType == BlockType::AICUBE) {
            it->aicBlocks.insert(error.auxData.coreId);
        } else {
            it->aicoreBlocks.insert(error.auxData.coreId);
        }
    }
}

};
