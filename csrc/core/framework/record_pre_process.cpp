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

#include "record_pre_process.h"

#include <functional>
#include <map>
#include <mutex>
#include <utility>
#include <sstream>
#include "record_format.h"
#include "constant.h"
#include "utility/log.h"
#include "platform_config.h"
#include "record_parse.h"
#include "format_converter.h"
#include "runtime_context.h"

namespace Sanitizer {
std::mutex RecordPreProcess::mtx_;

bool IgnoreSoftSyncRecord(const KernelRecord &kernelRecord)
{
    bool ignoreSoftSyncTag{};
    RecordType type = kernelRecord.recordType;
    if (type == RecordType::IB_SET_STUB || type == RecordType::IB_WAIT_STUB || type == RecordType::SYNC_ALL_STUB) {
        ignoreSoftSyncTag = true;
    }
    return ignoreSoftSyncTag;
}

bool IgnoreMstxCrossSetRecord(const KernelRecord &kernelRecord)
{
    bool ignoreMstxSetTag{};
    RecordType type = kernelRecord.recordType;
    if (type == RecordType::MSTX_STUB && kernelRecord.payload.mstxRecord.interfaceType ==
        InterfaceType::MSTX_SET_CROSS_SYNC) {
        ignoreMstxSetTag  = kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.isMerge;
    }
    return ignoreMstxSetTag;
}

/// 目前只有IB_SET_STUB、IB_WAIT_STUB、SYNC_ALL_STUB以及MSTX_STUB中的interfaceId = 0支持合并模式
bool IgnoreRecord(const KernelRecord &kernelRecord)
{
    return IgnoreSoftSyncRecord(kernelRecord) || IgnoreMstxCrossSetRecord(kernelRecord);
}

void RecordPreProcess::UpdateMergeInfo(const KernelRecord &kernelRecord)
{
    if (IgnoreRecord(kernelRecord)) {
        /// 如果是可以合并的记录，则翻转merge标志位；
        mstxMergeTag_ = !mstxMergeTag_;
    }
}

bool IsRealSyncType(RecordType type)
{
    return (
            (type == RecordType::SET_FLAG) ||
            (type == RecordType::WAIT_FLAG) ||
            (type == RecordType::HSET_FLAG) ||
            (type == RecordType::HWAIT_FLAG) ||
            (type == RecordType::PIPE_BARRIER) ||
            (type == RecordType::FFTS_SYNC) ||
            (type == RecordType::WAIT_FLAG_DEV) ||
            (type == RecordType::SET_ATOMIC)
        );
}

bool IsMstxWaitRecord(const KernelRecord &kernelRecord)
{
    return kernelRecord.recordType == RecordType::MSTX_STUB &&
           kernelRecord.payload.mstxRecord.interfaceType == InterfaceType::MSTX_WAIT_CROSS_SYNC &&
           kernelRecord.payload.mstxRecord.interface.mstxCrossRecord.isMore;
}

void RecordPreProcess::Parse(const SanitizerRecord &record, std::vector<SanEvent> &events)
{
    RecordParse::Parse(record, events);
}

bool CrossWaitRecordIsEqual(const MstxCrossWaitRecordInfo &lhs, const MstxCrossRecord &rhs, uint32_t coreID)
{
    return lhs.record == rhs && lhs.coreID == coreID;
}

RecordPreProcess &RecordPreProcess::GetInstance()
{
    thread_local static RecordPreProcess inst;
    return inst;
}

/// mstxCrossRecord 为一个循环队列，队列长度为42，队列中通信地址各不相同；具体逻辑如下：
/// set1，wait1-1，wait1-2，wait1-3，wait1-4 ......
/// set2, wait2-1，wait2-2，wait2-3 ......
/// ......
/// set41, wait41-1，wait41-2，wait41-3 ......
/// set1，wait1-1，wait1-2，wait1-3 ......
/// 对于同一个地址，同一个wait会被调用多次，并且次数不固定，并且只有一个set与之对应；
/// 代码处理逻辑为将多个wait中间所有的记录忽略，所有记录合并为单个wait；
void RecordPreProcess::ProcessMstxCrossWaitRecord(const SanitizerRecord &record)
{
    recordBuffer_.push_back(record);
    if (!waitMergeInfo_.throughFirstWait) {
        /// 如果没有经过第一个wait，则buffer缓存完成；
        waitMergeInfo_.endTag = true;
    } else {
        /// 越过了第一个wait之后，后续所有的记录开启缓存标识；
        waitMergeInfo_.endTag = false;
    }

    if (record.version == RecordVersion::KERNEL_RECORD) {
        auto &kernelRecord = record.payload.kernelRecord;
        auto type = kernelRecord.recordType;
        if (IsMstxWaitRecord(kernelRecord)) {
            /// 此判断条件保证了进入之后记录为需要处理的WaitRecord
            auto &curMstxRecord = kernelRecord.payload.mstxRecord;

            bool fistEqual = waitMergeInfo_.throughFirstWait && CrossWaitRecordIsEqual(waitMergeInfo_.firstPreRecord,
                curMstxRecord.interface.mstxCrossRecord, curMstxRecord.location.blockId);
            bool secondEqual = waitMergeInfo_.throughSecondWait && CrossWaitRecordIsEqual(
                waitMergeInfo_.secondPreRecord, curMstxRecord.interface.mstxCrossRecord,
                curMstxRecord.location.blockId);
            /// 更新merge标志位；
            if (fistEqual || secondEqual) {
                /// 如果当前的waitRecord和前一个或前前一个记录相等，则表示当前wait被调用了多次，为多余指令，可以删除；
                /// 同时将缓存标志位置为false，表示后续记录会被继续缓存；
                recordBuffer_.pop_back();
                waitMergeInfo_.endTag = false;
            } else if (!waitMergeInfo_.throughFirstWait || !waitMergeInfo_.throughSecondWait) {
                /// 此分支表示遇到了第1个或第2个wait，此时缓存标志位置为false，表示后续记录会继续缓存；
                waitMergeInfo_.endTag = false;
            } else {
                /// 此分支表示已经越过了第一个和第二个wait，并且当前记录表示不同的wait，则缓存标志位置为true，表示缓存完成；
                waitMergeInfo_.endTag = true;
            }

            /// 更新waitMergeInfo_信息；
            if (waitMergeInfo_.throughSecondWait) {
                /// 如果遇到了第2个wait，则表达当前的wait为第3个及之后的wait；
                waitMergeInfo_.firstPreRecord = waitMergeInfo_.secondPreRecord;
                waitMergeInfo_.secondPreRecord.record = curMstxRecord.interface.mstxCrossRecord;
                waitMergeInfo_.secondPreRecord.coreID = curMstxRecord.location.blockId;
            } else if (waitMergeInfo_.throughFirstWait) {
                /// 如果没遇到第2个wait并且遇到了第1个wait，则表示当前wait为第2个wait；
                waitMergeInfo_.secondPreRecord.record = curMstxRecord.interface.mstxCrossRecord;
                waitMergeInfo_.secondPreRecord.coreID = curMstxRecord.location.blockId;
                waitMergeInfo_.throughSecondWait = true;
            } else {
                /// 没有遇到第1个wait也没有遇到第2个wait，则表示当前wait为第1个wait;
                waitMergeInfo_.firstPreRecord.record = curMstxRecord.interface.mstxCrossRecord;
                waitMergeInfo_.firstPreRecord.coreID = curMstxRecord.location.blockId;
                waitMergeInfo_.throughFirstWait = true;
            }
        } else if (type == RecordType::BLOCK_FINISH || type == RecordType::KERNEL_FINISH) {
            /// 重置waitMergeInfo_信息；
            ResetWaitMergeInfo();
        }
    }
}

void RecordPreProcess::Process(const SanitizerRecord &record, std::vector<SanEvent> &events)
{
    if (record.version == RecordVersion::MEMORY_RECORD) {
        return;
    }
    // 根据不同的RecordType选取不同的函数处理
    KernelRecord const &kernelRecord = record.payload.kernelRecord;

    // 每个SYNC_ALL_STUB指令对的第一个指令翻转为true
    if (kernelRecord.recordType == RecordType::SYNC_ALL_STUB) {
        isFirstSyncAllStub_ = !isFirstSyncAllStub_;
    }

    UpdateMergeInfo(kernelRecord);

    // 处于syncall合并区间时，仅保留真实的硬件同步指令；
    // 例外：第一条SYNC_ALL_STUB（边界标记）、KERNEL_FINISH/BLOCK_FINISH（标识结束，需恢复状态）不可丢弃
    if (mstxMergeTag_ && !IsRealSyncType(kernelRecord.recordType) &&
        !(kernelRecord.recordType == RecordType::SYNC_ALL_STUB && isFirstSyncAllStub_) &&
        kernelRecord.recordType != RecordType::KERNEL_FINISH && kernelRecord.recordType != RecordType::BLOCK_FINISH) {
        return;
    }

    // 算子finish/block结束时恢复标记位状态，避免死循环状态残留
    if (kernelRecord.recordType == RecordType::KERNEL_FINISH || kernelRecord.recordType == RecordType::BLOCK_FINISH) {
        mstxMergeTag_ = false;
        isFirstSyncAllStub_ = false;
    }

    ProcessMstxCrossWaitRecord(record);
    if (!waitMergeInfo_.endTag) {
        return;
    }

    for (auto const &rec : recordBuffer_) {
        Parse(rec, events);
    }
    ClearBuffer();
}

void RecordPreProcess::ResetWaitMergeInfo()
{
    waitMergeInfo_.throughFirstWait = false;
    waitMergeInfo_.throughSecondWait = false;
    waitMergeInfo_.endTag = true;
}

void RecordPreProcess::ClearBuffer()
{
    recordBuffer_.clear();
}

}
