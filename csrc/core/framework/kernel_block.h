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


#pragma once

#include <cstdint>
#include <memory>

#include "record_defs.h"

namespace Sanitizer {

class KernelBlock {
public:
    static std::unique_ptr<KernelBlock> CreateKernelBlock(uint8_t const *memInfo, uint32_t blockIdx);
    void ParseSimtRecord(std::vector<KernelRecord> &kernelRecords);
    void PrintCacheSizeLog(uint64_t totalSize);
    bool NextSimd(KernelRecord &kernelRecord);
    void ParseShadowMemoryRecord(std::vector<KernelRecord> &kernelRecords);

    uint64_t GetTotalBlockDim() const;

    static void ResetAll()
    {
        vecSubBlockDim_ = {};
        totalBlockDim_ = {};
        dynamicMemorys_ = {};
    }

private:
    bool ParseSimdRecord(uint8_t const *record, KernelRecord &kernelRecord);
    std::vector<uint8_t> AllocMemory(size_t size) const;

    /**
     * @brief 申请动态内存存放动态内存记录，并将动态记录放入kernelRecords中
     * @param recordCount      动态记录包含的单个内存记录数量
     * @param kernelRecord     动态记录对应的kernelRecord
     * @param kernelRecords    kernel侧记录缓存队列
     * @param offset           动态记录相较于起始地址的偏移
     */
    void PushShadowMemoryRecord(size_t recordCount, KernelRecord &kernelRecord,
        std::vector<KernelRecord> &kernelRecords, uint32_t offset = 0) const;

    RecordGlobalHead recordGlobalHead_ {};
    RecordBlockHead simdRecordHead_ {};
    SimtRecordBlockHead const *simtRecordHead_ {};
    ShadowMemoryRecordHead const *shadowMemoryHead_ {};
    uint8_t const *simdRecords_ {};
    uint8_t const *simtRecords_ {};
    uint64_t simdOffset_ {};
    uint64_t simtOffset_ {};
    uint64_t recordIdx_ {};
    thread_local static uint8_t vecSubBlockDim_;       // mix算子vec核使用的子核数目，只记录在了vec 0核的头部
    thread_local static uint64_t totalBlockDim_;       // 记录的总blockDim数，只记录在了0核的头部
    thread_local static std::vector<std::vector<uint8_t>> dynamicMemorys_;       // 动态内存记录缓存数组
    uint64_t extendCacheSize_{};                       // 溢出的cacheSize
    uint64_t extendRecordCount_{};                     // 溢出的记录数量
    uint32_t blockIdx_{};
};

} // namespace Sanitizer
