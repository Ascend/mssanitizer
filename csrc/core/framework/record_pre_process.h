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


#ifndef CORE_FRAMEWORK_RECORD_PRE_PROCESS_H
#define CORE_FRAMEWORK_RECORD_PRE_PROCESS_H

#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "utility/cpp_future.h"
#include "record_defs.h"
#include "event_def.h"

namespace Sanitizer {

 // 由于MstxCrossRecord中没有coreID字段，该字段用于标识preRecord对应的的coreID;
struct MstxCrossWaitRecordInfo {
    MstxCrossRecord record{};        // 当前的record
    uint32_t coreID{};               // 记录对应的coreID
};

struct MstxCrossWaitMergeInfo {
    MstxCrossWaitRecordInfo firstPreRecord{};  // 前一个 MstxCrossRecord
    MstxCrossWaitRecordInfo secondPreRecord{}; // 前前一个 MstxCrossRecord
    bool throughFirstWait{};                   // 用于标记是否已经遇到了第1个wait；
    bool throughSecondWait{};                  // 用于标记是否已经遇到了第2个wait；
    bool endTag{};                             // 缓存记录的标识符，如果为true，则recordBuffer缓存完成，为false则会继续缓存；
};

// 内存检测算法预处理类，将桩原始信息转换成算法依赖的内存操作信息
class RecordPreProcess {
public:
    static RecordPreProcess &GetInstance();
    //  内存检测算法预处理
    void Parse(const SanitizerRecord &record, std::vector<SanEvent> &events);
    void Process(const SanitizerRecord &record, std::vector<SanEvent> &events);

    static std::mutex mtx_;

private:
    void ProcessMstxCrossWaitRecord(const SanitizerRecord &record);

    /// 重置合并标志信息；
    void ResetWaitMergeInfo();

    /// 清空buffer
    void ClearBuffer();

    void UpdateMergeInfo(const KernelRecord &kernelRecord);

    template<class T, class... Args>
    friend std::unique_ptr<T> MakeUnique(Args &&... args);

    RecordPreProcess() {};

private:
    std::vector<SanitizerRecord> recordBuffer_;               // 缓存记录，主要用于处理有些记录需要依赖后续的记录才能决定当前记录是否保留；
    MstxCrossWaitMergeInfo waitMergeInfo_;                    // 缓存记录对应的标志信息
    bool mstxMergeTag_{};                                     // mstx记录合并标志位，为true时合并记录
};

}  // namespace Sanitizer

#endif
