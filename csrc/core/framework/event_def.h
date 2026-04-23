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


#ifndef CORE_FRAMEWORK_EVENT_DEF_H
#define CORE_FRAMEWORK_EVENT_DEF_H

#include <array>
#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include "arch_def.h"
#include "record_defs.h"
#include "sanitizer_report.h"
#include "utility/log.h"

namespace Sanitizer {

// 该头文件定义承载竞争检测算法所需的基础架构,数据类型

enum class EventType : uint8_t {
    SANITIZER_CONTROL_EVENT,
    MEM_EVENT,
    SYNC_EVENT,
    TIME_EVENT,
    CROSS_CORE_SYNC_EVENT,
    CROSS_CORE_SOFT_SYNC_EVENT,
    MSTX_CROSS_SYNC_EVENT,
    REGISTER_EVENT,
    H_SYNC_EVENT,
    BUF_SYNC_EVENT,
    MSTX_SIGNAL_SET_EVENT,
    MSTX_SIGNAL_WAIT_EVENT,
    MSTX_CROSS_CORE_BARRIER,
    MSTX_CROSS_NPU_BARRIER,
    DYNAMIC_MEM_EVENT,
};

enum class SanitizerControlType : uint8_t {
    KERNEL_FINISH = 0,
    FINISH
};

// 算法预处理阶段hset_flag/hwait_flag处理成普通的set_flag/wait_flag
enum class SyncType : uint8_t {
    SET_FLAG = 0U,
    WAIT_FLAG,
    PIPE_BARRIER,
    FFTS_SYNC,
    WAIT_FLAG_DEV,
    IB_SET,
    IB_WAIT,
    SYNC_ALL,
    MSTX_SET_CROSS,
    MSTX_WAIT_CROSS,
    GET_BUF,
    RLS_BUF,
    WAIT_INTRA_BLOCK,
    HSET_FLAG,
    HWAIT_FLAG,
};

enum class RaceCheckType: uint8_t {
    SINGLE_BLOCK_CHECK = 0U,
    SINGLE_PIPE_CHECK,
    CROSS_BLOCK_CHECK,
    CROSS_NPU_CHECK,
};

enum class SyncCheckType : uint8_t {
    MATCH_CHECK = 0U,    // set_flag 匹配检测
    REDUMTAMCY_CHECK,   // set_flag/wait_flag 冗余检测
    SIZE,
};

enum class FftsSyncMode : uint8_t {
    MODE0 = 0U,
    MODE1,
    MODE2,
    MODE3,
    MODE4,
};

struct SanitizerControlInfo {
    SanitizerControlType type;
};

struct MemOpInfo {
    MemType memType;
    AccessType opType;
    // vectorMask/maskMode/dataBits均是描述掩码的参数，在maskmode为MASK_NORM时均不生效
    VectorMask vectorMask;
    MaskMode maskMode;
    uint8_t dataBits;

    uint64_t addr;
    // 由于内存操作的地址不一定连续,这里通过blockNum/blockSize/blockStride来描述一次内存操作的"length"
    uint32_t blockNum;
    uint32_t blockSize;
    uint32_t blockStride;
    uint32_t repeatTimes;
    uint32_t repeatStride;
    // 对齐大小，由内存检测引入
    uint16_t alignSize;
    bool ignoreIllegalCheck;
};

struct SyncOpInfo {
    SyncType opType;
    PipeType srcPipe;
    PipeType dstPipe;
    uint32_t eventId;
    MemType memType;
    bool isRetrogress;  // 是否由HSet/HWait退化，避免混用导致解析顺序混乱
    bool isGenerated;  // 是否是生成的非原生指令
};

struct HSyncOpInfo {
    uint64_t eventId;
    PipeType srcPipe;
    PipeType dstPipe;
    SyncType opType;
    MemType memType;
    uint8_t v;
    bool isReplaced;
};

struct FftsSyncInfo {
    SyncType opType;
    PipeType dstPipe;
    uint8_t flagId;
    uint8_t mode;
    uint8_t vecSubBlockDim;
};

struct BufSyncInfo {
    SyncType opType;
    PipeType pipe;
    uint64_t bufId;
    uint64_t rlsCount;  // get_buf需要等待的rls_buf数量
    BufMode mode;
};

struct SoftSyncInfo {
    SyncType opType;
    int32_t eventID;
    uint16_t waitCoreID; // 被等的核ID
    int32_t usedCores;
    bool isAIVOnly;
};

struct MstxCrossInfo {
    uint64_t addr;
    uint64_t flagId;
    PipeType pipe;
    bool isMore;
    bool isMerge;
    SyncType opType;
};

struct AtomicModeInfo {
    AtomicMode mode;
};

struct RegisterOpInfo {
    RegisterType regType;
    RegisterPayload regPayLoad;
};

struct DynamicOpInfo {
    uint64_t count{};            // buffer包含多少个结构体
    uint64_t minAddr{};          // 变长协议的最小地址
    uint64_t maxAddr{};          // 变长协议的最大地址
    RecordType dynamicType{};    // 表示buffer对应的结构体类型
    MemType memType{};           // 变长协议对应的内存类型
    void *buffer{nullptr};       // 变长协议对应的协议内容
};

using VectorTime = std::vector<uint32_t>;

inline MemType AddrSpaceToMemType(AddressSpace addrSpace)
{
    static const std::map<AddressSpace, MemType> ADDR_SPACE_TO_MEM_TYPE_MAP = {
        { AddressSpace::L1, MemType::L1 },
        { AddressSpace::L0A, MemType::L0A },
        { AddressSpace::L0B, MemType::L0B },
        { AddressSpace::L0C, MemType::L0C },
        { AddressSpace::UB, MemType::UB },
        { AddressSpace::GM, MemType::GM },
        { AddressSpace::BT, MemType::BT },
        { AddressSpace::FB, MemType::FB },
        { AddressSpace::INVALID, MemType::INVALID },
        { AddressSpace::PRIVATE, MemType::PRIVATE },
    };
    auto it = ADDR_SPACE_TO_MEM_TYPE_MAP.find(addrSpace);
    return it == ADDR_SPACE_TO_MEM_TYPE_MAP.cend() ? MemType::INVALID : it->second;
}

struct LocInfo {
    uint64_t fileNo;
    uint64_t lineNo;
    uint64_t pc;
    uint32_t deviceIdx;
    uint32_t kernelIdx;
    uint32_t deviceId;
    uint32_t coreId;
    BlockType blockType;
};

// 竞争/内存处理的基础元素类型就是事件
struct SanEvent {
    uint64_t serialNo{};
    EventType type{};
    PipeType pipe{};
    union {
        SanitizerControlInfo sanitizerControlInfo;
        SyncOpInfo syncInfo;
        MemOpInfo memInfo;
        FftsSyncInfo fftsSyncInfo;
        BufSyncInfo bufSyncInfo;
        SoftSyncInfo softSyncInfo;
        MstxCrossInfo mstxCrossInfo;
        AtomicModeInfo atomicModeInfo;
        RegisterOpInfo regInfo;
        HSyncOpInfo hsyncInfo;
        MstxSignalSet mstxSignalSet;
        MstxSignalWait mstxSignalWait;
        MstxCrossCoreBarrier mstxCrossCoreBarrier;
        MstxCrossNpuBarrier mstxCrossNpuBarrier;
        DynamicOpInfo dynamicOpInfo;
    } eventInfo{};
    VectorTime timeInfo;
    LocInfo loc{};
    bool isAtomicMode = false;
};

// 输入数据结构
struct MemEvent {
    uint64_t serialNo = 0U;
    uint64_t barrierNo = 0U;
    uint64_t pipeSerialNo = 0U;
    VectorTime vt;
    MemOpInfo memInfo;
    DynamicOpInfo dynamicMemInfo;
    LocInfo loc;
    PipeType pipe;
    bool isAtomicMode = false;
    bool isDynamic = false;

    explicit MemEvent(const SanEvent &event)
        : serialNo(event.serialNo), loc(event.loc), pipe(event.pipe),
        isAtomicMode(event.isAtomicMode)
    {
        if (event.type == EventType::DYNAMIC_MEM_EVENT) {
            dynamicMemInfo = event.eventInfo.dynamicOpInfo;
            isDynamic = true;
        } else {
            memInfo = event.eventInfo.memInfo;
            isDynamic = false;
        }
    }
};

struct ErrorEvent {
    uint64_t serialNo;
    uint32_t deviceId;
    uint32_t kernelIdx;
    uint32_t coreId;
    uint64_t addr;
    uint64_t fileNo;
    uint64_t lineNo;
    uint64_t pc;
    uint8_t accessType;
    uint8_t memType;
    uint8_t pipeType;
    BlockType blockType;
    SimtThreadLocation threadLoc;
    bool isSimt;

    void Init(const MemEvent &memEvent, uint64_t dynamicErrIdx = 0)
    {
        serialNo = memEvent.serialNo;
        deviceId = memEvent.loc.deviceId;
        kernelIdx = memEvent.loc.kernelIdx;
        coreId = memEvent.loc.coreId;
        blockType = memEvent.loc.blockType;
        fileNo = memEvent.loc.fileNo;
        lineNo = memEvent.loc.lineNo;
        pipeType = static_cast<uint8_t>(memEvent.pipe);
        if (memEvent.isDynamic) {
            auto &dynamicMemInfo = memEvent.dynamicMemInfo;
            if (dynamicMemInfo.count > 0 && dynamicMemInfo.buffer != nullptr && dynamicErrIdx < dynamicMemInfo.count &&
                dynamicMemInfo.dynamicType == RecordType::SHADOW_MEMORY) {
                auto errorRecord = reinterpret_cast<const ShadowMemoryRecord *>(dynamicMemInfo.buffer)[dynamicErrIdx];
                addr = errorRecord.addr;
                pc = errorRecord.location.pc;
                threadLoc = errorRecord.threadLoc;
                isSimt = true;
                accessType = static_cast<uint8_t>(errorRecord.accessType);
                memType = static_cast<uint8_t>(AddrSpaceToMemType(errorRecord.space));
                return;
            }
        }
        addr = memEvent.memInfo.addr;
        pc = memEvent.loc.pc;
        threadLoc = {};
        isSimt = false;
        accessType = static_cast<uint8_t>(memEvent.memInfo.opType);
        memType = static_cast<uint8_t>(memEvent.memInfo.memType);
    }

    bool operator == (const ErrorEvent &other) const
    {
        return (accessType == other.accessType &&
                memType == other.memType &&
                addr == other.addr &&
                fileNo == other.fileNo &&
                lineNo == other.lineNo &&
                pc == other.pc &&
                pipeType == other.pipeType &&
                coreId == other.coreId &&
                threadLoc == other.threadLoc &&
                isSimt == other.isSimt);
    }

    bool IsSameSimt(const ErrorEvent &other) const {
         return (coreId == other.coreId &&
                addr == other.addr &&
                fileNo == other.fileNo &&
                lineNo == other.lineNo &&
                pc == other.pc &&
                accessType == other.accessType &&
                memType == other.memType &&
                pipeType == other.pipeType &&
                blockType == other.blockType &&
                isSimt == other.isSimt);
    }
};

// 竞争检测信息展示单元
struct RaceDispInfo {
    ErrorEvent p1, p2;

    bool IsSameSimt(const RaceDispInfo &other) const {
        return (p1.IsSameSimt(other.p1) && p2.IsSameSimt(other.p2)) ||
               (p1.IsSameSimt(other.p2) && p2.IsSameSimt(other.p1));
    }

    void UpdateMinThreadLoc(const RaceDispInfo &other) {
        std::vector<SimtThreadLocation> simtLocs;
        simtLocs.emplace_back(p1.threadLoc);
        simtLocs.emplace_back(p2.threadLoc);
        simtLocs.emplace_back(other.p1.threadLoc);
        simtLocs.emplace_back(other.p2.threadLoc);
        std::sort(simtLocs.begin(), simtLocs.end());
        auto last = std::unique(simtLocs.begin(), simtLocs.end());
        simtLocs.erase(last, simtLocs.end());
        if (simtLocs.size() >= 2) {
            p1.threadLoc = simtLocs[0];
            p2.threadLoc = simtLocs[1];
        } else {
            SAN_ERROR_LOG("simt locations size error, size = %lu", simtLocs.size());
        }
    }
};

// 同步检测信息展示单元
struct SyncDispInfo {
    ErrorEvent baseEvent;
    PipeType srcPipe;
    PipeType dstPipe;
    uint32_t eventId;
    SyncType opType;
    SyncCheckType checkType;

    bool operator == (const SyncDispInfo &other) const
    {
        return (baseEvent.coreId == other.baseEvent.coreId &&
                baseEvent.blockType == other.baseEvent.blockType &&
                srcPipe == other.srcPipe &&
                dstPipe == other.dstPipe &&
                eventId == other.eventId &&
                opType == other.opType &&
                checkType == other.checkType);
    }
};

struct RegisterDispInfo {
    ErrorEvent baseEvent;
    std::string kernelName = "UNKNOWN KERNEL";
    RegisterType regType;    // 未重置的寄存器的名称
    RegisterPayload regExpVal;  // 寄存器预期的默认值
    RegisterPayload regActVal;  // 寄存器实际值
};

struct ErrorEventHash {
    size_t operator () (const RaceDispInfo &it) const
    {
        return std::hash<uint64_t>()(it.p1.addr) ^ std::hash<uint64_t>()(it.p2.addr) ^
            std::hash<uint64_t>()(it.p1.fileNo) ^ std::hash<uint64_t>()(it.p2.fileNo) ^
            std::hash<uint64_t>()(it.p1.lineNo) ^ std::hash<uint64_t>()(it.p2.lineNo) ^
            std::hash<uint64_t>()(it.p1.pc) ^ std::hash<uint64_t>()(it.p2.pc) ^
            std::hash<uint8_t>()(it.p1.accessType) ^ std::hash<uint8_t>()(it.p1.memType) ^
            std::hash<uint8_t>()(it.p2.accessType) ^ std::hash<uint8_t>()(it.p2.memType) ^
            std::hash<uint8_t>()(it.p1.pipeType) ^ std::hash<uint8_t>()(it.p2.pipeType);
    }
};

struct ErrorEventEqual {
    bool operator () (const RaceDispInfo &rd1, const RaceDispInfo &rd2) const noexcept
    {
        return (rd1.p1 == rd2.p1 && rd1.p2 == rd2.p2) || (rd1.p1 == rd2.p2 && rd1.p2 == rd2.p1);
    }
};

}
#endif
