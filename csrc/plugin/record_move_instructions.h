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

#ifndef PLUGIN_RECORD_MOVE_INSTRUCTIONS_H
#define PLUGIN_RECORD_MOVE_INSTRUCTIONS_H

#include <utility>
#include "kernel_pub_func.h"
#include "utils.h"
#include "recorder.h"
#include "addr_process.h"
#include "plugin/record_ctrl_instructions.h"

namespace Sanitizer {

AICORE_FUNC_HEAD uint64_t StackAddrTransform(uint64_t addr)
{
    // 本函数把栈地址转换为从0起始的偏移量，便于后续算法处理
    // 转换策略：转换后地址 = 原始栈地址 - 对应芯片架构设置的栈空间起始偏移

    // 根据SU FS, A2/A3/A5栈地址的范围如下所示:
    // SYS_VA_BASE + 0x100000 + (CoreID * Stack Size) ~ SYS_VA_BASE + 0x100000 + (CoreID * Stack Size) + Stack Size
    // 310P的地址范围为0x40000 ~ 0x40000 + 0x4000
    uint64_t transformedAddr = addr;
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    transformedAddr = addr - GetSysVaBase() - 0x100000; // 对地址做偏移，偏移后的值作为后续的地址输入。
#elif defined(__DAV_M200__) || (defined __DAV_M200_VEC__)
    transformedAddr = addr - 0x40000;
#endif
    return transformedAddr;
}

template<RecordType recordType, DetailedDataType dataType = DetailedDataType::Default>
AICORE_FUNC_HEAD void RecordLoadStoreEvent(EXTRA_PARAMS_DEC, AddressSpace space, uint64_t addr,
                                            uint64_t size, uint8_t alignSize)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    /// 记录para_base寄存器的地址，防止load para内存时误报
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    recorder.SetParaBaseAddr(get_para_base());
// 目前只有c310的动态插桩会启动extra的信息写入
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__) && defined(BUILD_DYNAMIC_PROBE)
    if (recordType == RecordType::LDP || recordType == RecordType::LD || recordType == RecordType::LD_IO ||
        recordType == RecordType::LD_DEV) {
        recorder.ProcessParaBaseAddr();
    }
#endif // __NPU_ARCH__
#endif // __CCE_IS_AICORE__

    // 编译器那边容易把stack上的gm地址推断成PRIVATE，这里做一个补充
    if (space == AddressSpace::PRIVATE) {
        uint64_t sysVaBase = GetSysVaBase();
        if (GetUintFromConf<48, 25>(addr) != GetUintFromConf<48, 25>(sysVaBase) || GetUintFromConf<24>(addr) == 1) {
            space = AddressSpace::GM;
        }
    }
    if (space == AddressSpace::PRIVATE) {
        auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
        /// 不开启初始化检测时，过滤掉栈上的LOAD/STORE记录
        if (!head->checkParms.initcheck) {
            return;
        }
        addr = StackAddrTransform(addr);
    }

    auto record = LoadStoreRecord{};
    record.addr = addr;
    /// 有同名函数时，mix算子链接时会优先链接cube.o中的函数，会导致mix算子load/store指令越界，这里需保证vec/cube函数逻辑相同
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__) || \
    (defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510))
    if (space == AddressSpace::UB) {
        record.addr = GetUintFromConf<18, 0>(record.addr);
    }
#endif
    record.size = size;
    record.alignSize = alignSize;
    record.location.blockId = blockIdx;
    record.space = space;
    record.dataType = dataType;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    recorder.DumpRecord<recordType>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordDmaMovEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint8_t sid, uint16_t nBurst, uint16_t lenBurst,
                                         uint16_t srcStride, uint16_t dstStride, PadMode padMode, ByteMode byteMode)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = DmaMovRecord{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.srcStride = srcStride;
    record.dstStride = dstStride;
    record.padMode = padMode;
    record.byteMode = byteMode;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::DMA_MOV>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordDmaMovEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config, PadMode padMode, ByteMode byteMode)
{
    uint8_t sid = config & 0xF;
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcStride = (config >> 32) & 0xFFFF;
    uint16_t dstStride = (config >> 48) & 0xFFFF;

    RecordDmaMovEvent<srcMemType, dstMemType>(EXTRA_PARAMS, dst, src, sid, nBurst, lenBurst, srcStride,
                                              dstStride, padMode, byteMode);
}

template<MemType srcMemType, MemType dstMemType, DataType srcDataType, DataType dstDataType, RecordType recordType>
AICORE_FUNC_HEAD void RecordDmaMovConvReluEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint8_t sid,
                                                uint16_t nBurst, uint16_t lenBurst, uint16_t srcStride,
                                                uint16_t dstStride, ConvRelu crMode)
{
    (void)sid;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = DmaMovConvReluRecord{};
    record.dst = dst;
    record.src = src;
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.srcStride = srcStride;
    record.dstStride = dstStride;
    record.location.blockId = blockIdx;
    record.crMode = crMode;
    record.dstMemType = dstMemType;
    record.srcMemType = srcMemType;
    record.srcDataType = srcDataType;
    record.dstDataType = dstDataType;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType srcDataType, DataType dstDataType, RecordType recordType>
AICORE_FUNC_HEAD void RecordDmaMovConvReluEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                                uint64_t src, uint64_t config, ConvRelu crMode)
{
    uint8_t sid = config & 0xF;
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcStride = (config >> 32) & 0xFFFF;
    uint16_t dstStride = (config >> 48) & 0xFFFF;

    RecordDmaMovConvReluEvent<srcMemType, dstMemType, srcDataType, dstDataType, recordType>(EXTRA_PARAMS, dst, src,
                                                                                sid, nBurst, lenBurst, srcStride,
                                                                                dstStride, crMode);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovNd2nzEvent(EXTRA_PARAMS_DEC,
                                              uint64_t dst, uint64_t src, uint16_t ndNum, uint16_t nValue,
                                              uint16_t dValue, uint16_t srcNdMatrixStride, uint16_t srcDValue,
                                              uint16_t dstNzC0Stride, uint16_t dstNzNStride,
                                              uint16_t dstNzMatrixStride)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = DmaMovNd2nzRecord {};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.ndNum = ndNum;
    record.nValue = nValue;
    record.dValue = dValue;
    record.srcNdMatrixStride = srcNdMatrixStride;
    record.srcDValue = srcDValue;
    record.dstNzC0Stride = dstNzC0Stride;
    record.dstNzNStride = dstNzNStride;
    record.dstNzMatrixStride = dstNzMatrixStride;

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    record.location.blockId = blockIdx;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::DMA_MOV_ND2NZ>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovNd2nzEvent(EXTRA_PARAMS_DEC,
                                              uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    uint16_t ndNum = (xm >> 4) & 0xFFF;
    uint16_t nValue = (xm >> 16) & 0xFFFF;
    uint16_t dValue = (xm >> 32) & 0xFFFF;
    uint16_t srcNdMatrixStride = (xm >> 48) & 0xFFFF;

    uint16_t srcDValue = xt & 0xFFFF;
    uint16_t dstNzC0Stride = (xt >> 16) & 0xFFFF;
    uint16_t dstNzNStride = (xt >> 32) & 0xFFFF;
    uint16_t dstNzMatrixStride = (xt >> 48) & 0xFFFF;

    RecordDmaMovNd2nzEvent<srcMemType, dstMemType, dataType>(EXTRA_PARAMS, dst, src, ndNum, nValue, dValue,
                                                             srcNdMatrixStride, srcDValue, dstNzC0Stride, dstNzNStride,
                                                             dstNzMatrixStride);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType, RecordType recordType>
AICORE_FUNC_HEAD void DumpDmaMovNd2nzDavRecord(EXTRA_PARAMS_DEC,
    uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    DmaMovNd2nzDavRecord record{};
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.location.blockId = static_cast<uint16_t>(blockIdx);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.loop1SrcStride = GetUintFromConf<43, 4>(xm);
    record.nValue = GetUintFromConf<63, 48>(xm);
    record.dValue = GetUintFromConf<20, 0>(xt);
    record.loop4SrcStride = GetUintFromConf<60, 21>(xt);
    record.smallC0 = GetUintFromConf<61, 61>(xt);

    Recorder recorder(memInfo, blockIdx);
    uint64_t sprMte2NzPara{};
    recorder.GetRegister(&Register::sprMte2NzPara, sprMte2NzPara);
    record.ndNum = GetUintFromConf<15, 0>(sprMte2NzPara);
    record.loop2DstStride = GetUintFromConf<31, 16>(sprMte2NzPara);
    record.loop3DstStride = GetUintFromConf<47, 32>(sprMte2NzPara);
    record.loop4DstStride = GetUintFromConf<63, 48>(sprMte2NzPara);
    recorder.DumpRecord<recordType>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovDn2nzEventDav(EXTRA_PARAMS_DEC,
                                                 uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    DumpDmaMovNd2nzDavRecord<srcMemType, dstMemType, dataType, RecordType::DMA_MOV_DN2NZ_D>(
        EXTRA_PARAMS, dst, src, xm, xt);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovNd2nzEventDav(EXTRA_PARAMS_DEC,
                                                 uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    DumpDmaMovNd2nzDavRecord<srcMemType, dstMemType, dataType, RecordType::DMA_MOV_ND2NZ_D>(
        EXTRA_PARAMS, dst, src, xm, xt);
}

// 与v1版本差异为v2无byteMode，所以复用v1的记录方法
template <MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovV2Event(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint8_t sid, uint32_t nBurst,
    uint32_t lenBurst, PadMode padFuncMode, uint8_t l2Ctrl, uint64_t srcStride, uint32_t dstStride)
{
    ByteMode byteMode = ByteMode::BM_DISABLE;
    RecordDmaMovEvent<srcMemType, dstMemType>(
        EXTRA_PARAMS, dst, src, sid, nBurst, lenBurst, srcStride, dstStride, padFuncMode, byteMode);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovV2Event(EXTRA_PARAMS_DEC,
                                              uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    uint8_t sid = GetUintFromConf<3, 0>(xm);
    uint32_t nBurst = GetUintFromConf<20, 4>(xm);
    uint32_t lenBurst = GetUintFromConf<41, 25>(xm);
    PadMode padFuncMode = static_cast<PadMode>(GetUintFromConf<59, 56>(xm));
    uint8_t l2CacheCtrl = static_cast<uint8_t>(GetUintFromConf<63, 60>(xm));

    uint64_t srcStride = GetUintFromConf<35, 0>(xt);
    uint32_t dstStride = GetUintFromConf<56, 40>(xt);

    RecordDmaMovV2Event<srcMemType, dstMemType, dataType>(
        EXTRA_PARAMS, dst, src, sid, nBurst, lenBurst, padFuncMode, l2CacheCtrl, srcStride, dstStride);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config, AddrCalMode calMode = AddrCalMode::INC)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = Load2DRecord{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.baseIdx = config & 0xffff;
    record.repeat = (config >> 16U) & 0xff;
    record.srcStride = (config >> 24U) & 0xffff;
    // The input is a gap. The gap needs to be processed as a stride.
    record.dstStride = ((config >> 44U) & 0xffff) + 1U;
    record.blockSize = MATRIX_FRACTAL_SIZE;
    record.addrCalMode = calMode;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_2D>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DV2Event(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = Load2DRecord{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);

    Recorder recorder(memInfo, blockIdx);
    uint64_t mte2SrcPara = 0;
    recorder.GetRegister(&Register::sprMTE2SrcPara, mte2SrcPara);
    uint64_t tmpStride = mte2SrcPara & 0xFFFFFFFF; // 只取低32位
    int32_t srcStride = static_cast<int32_t>(tmpStride);
    uint32_t mStartPosition = config0 & 0xFFFFFFFF;
    uint32_t kStartPosition = (config0 >> 32) & 0xFFFFFFFF;
    uint16_t srcStrideAbs = srcStride > 0 ? srcStride : -srcStride;
    record.baseIdx = kStartPosition * srcStrideAbs + mStartPosition;
    uint16_t dstStride = config1 & 0xFFF;
    uint16_t mStep = (config1 >> 12) & 0xFFF;
    uint16_t kStep = (config1 >> 24) & 0xFFF;
    record.repeat = kStep;
    record.srcStride = srcStrideAbs;
    record.dstStride = dstStride;
    record.blockSize = MATRIX_FRACTAL_SIZE * mStep;
    record.addrCalMode = srcStride > 0 ? AddrCalMode::INC : AddrCalMode::DEC;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    recorder.DumpRecord<RecordType::LOAD_2D>(record);
}

template<MemType dstMemType, DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void RecordLoadL12DV2Event(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config0, uint64_t config1,
                                           TransposeMode transposeMode)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadL12DRecord{};
    record.dst = dst;
    record.src = src;
    record.mStartPosition = config0 & 0xFFFF;
    record.kStartPosition = (config0 >> 16) & 0xFFFF;
    record.mStep = (config0 >> 32) & 0xFF;
    record.kStep = (config0 >> 40) & 0xFF;
    record.srcStride = config1 & 0xFFFF;
    record.dstStride = (config1 >> 16) & 0xFFFF;
    record.detailedDataType = detailedDataType;
    record.transposeMode = transposeMode;

    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_L1_2D>(record);
}

// 目前MemType没有L0A(B)_MX，先设定成L0A(B)进行区分
template<MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoadL1Mx2DV2Event(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadL1Mx2DRecord{};
    record.dst = dst;
    record.src = src;
    record.xStartPosition = config0 & 0xFFFF;
    record.yStartPosition = (config0 >> 16) & 0xFFFF;
    record.xStep = (config0 >> 32) & 0xFF;
    record.yStep = (config0 >> 40) & 0xFF;
    record.srcStride = config1 & 0xFFFF;
    record.dstStride = (config1 >> 16) & 0xFFFF;

    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_L1_MX_2D>(record);
}

template<DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void RecordLoadL12DTransposeEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadL12DTransposeRecord{};
    record.dst = dst;
    record.src = src;
    record.repeat = (config0 >> 16) & 0xFF;
    record.srcStride = (config0 >> 24) & 0xFFFF;
    record.dstStride = ((config0 >> 44) & 0xFFFF) + 1;
    record.srcFracStride = ((config1 >> 16) & 0xFFFF) + 1;
    record.dstFracStride = (config1 & 0xFFFF) + 1;
    record.detailedDataType = detailedDataType;
    if (detailedDataType == DetailedDataType::B16) {
        record.srcFracStride = record.dstFracStride = 1;
    }

    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_L1_2D_TRANSPOSE>(record);
}

template<MemType srcMemType>
AICORE_FUNC_HEAD void RecordLoadSmaskEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                            uint64_t src, uint64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadSmaskRecord{};
    record.dst = dst;
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    /// 计算ceil({{Xt[11], Xt[6:0]}/16}*32B为实际搬运长度
    record.smaskSize = ((((config & 0x800) >> 4) | (config & 0x7f)) + 15U) / 16U * 32U;
    record.srcMemType = srcMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_SMASK>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint16_t baseIdx, uint8_t repeat, uint16_t srcStride,
                                         uint16_t dstStride, uint8_t sid, AddrCalMode calMode = AddrCalMode::INC)
{
    (void)sid;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = Load2DRecord{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.baseIdx = baseIdx;
    record.repeat = repeat;
    record.srcStride = srcStride;
    // The input is a gap. The gap needs to be processed as a stride.
    record.dstStride = dstStride;
    record.blockSize = MATRIX_FRACTAL_SIZE;
    record.addrCalMode = calMode;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_2D>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DSparseEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                               uint16_t startId, uint8_t repeat)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = Load2DSparseRecord{};
    record.dst = dst;
    record.src0 = src & 0xffffffff;
    record.src1 = (src >> 32U) & 0xffffffff;
    record.startId = startId;
    record.repeat = repeat;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_2D_SPARSE>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordLoad2DSparseEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t config)
{
    const uint16_t startId = config & 0xffff;
    const uint8_t repeat = (config >> 16U) & 0xff;

    RecordLoad2DSparseEvent<srcMemType, dstMemType>(EXTRA_PARAMS, dst, src, startId, repeat);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoad2DTransposeEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint16_t indexID,
                                         uint8_t repeat, uint16_t srcStride, uint16_t dstStride,
                                         bool addrmode, uint16_t dstFracStride)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = Load2DTransposeRecord{};
    record.dst = dst;
    record.src = src;
    record.indexId = indexID;
    record.repeat = repeat;
    record.srcStride = srcStride;
    // The input is a gap. The gap needs to be processed as a stride.
    record.dstStride = dstStride;
    record.addrMode = addrmode;
    record.dstFracStride = dstFracStride;
    record.dataType = dataType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_2D_TRANSPOSE>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoad2DTransposeEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config, uint64_t fracStride)
{
    const uint16_t indexId = config & 0xffff;
    const uint8_t repeat = (config >> 16U) & 0xff;
    const uint16_t srcStride = (config >> 24U) & 0xffff;
    // The input is a gap. The gap needs to be processed as a stride.
    const uint16_t dstStride = ((config >> 44U) & 0xffff);
    bool addrMode = (config >> 63U) & 0x1;
    const uint16_t dstFracStride = (fracStride & 0xffff);

    RecordLoad2DTransposeEvent<srcMemType, dstMemType, dataType>(EXTRA_PARAMS, dst, src, indexId, repeat, srcStride,
        dstStride, addrMode, dstFracStride);
}

template<MemType srcMemType>
AICORE_FUNC_HEAD void RecordDecompressHeaderEvent(EXTRA_PARAMS_DEC, uint64_t src, uint16_t nBlock, uint8_t sid)
{
    (void)sid;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = DecompressHeaderRecord{};
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.location.blockId = blockIdx;
    record.nBlock = nBlock;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::DECOMPRESS_HEADER>(record);
}

template<MemType srcMemType>
AICORE_FUNC_HEAD void RecordDecompressHeaderEvent(EXTRA_PARAMS_DEC, uint64_t src, uint64_t config)
{
    const uint16_t nBlock = config & 0x3ff;
    const uint8_t sid = (config >> 10U) & 0xf;
    RecordDecompressHeaderEvent<srcMemType>(EXTRA_PARAMS, src, nBlock, sid);
}

template<MemType srcMemType, MemType dstMemType, DataType srcDataType, DataType dstDataType>
AICORE_FUNC_HEAD void RecordBroadcastEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint8_t nBurst,
                                         uint8_t lenBurst, uint8_t srcGap, uint8_t dstGap, bool enableRepeat = false)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = BroadcastRecord{};
    record.dst = dst;
    record.src = src;
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.enableRepeat = enableRepeat;
    record.srcGap = srcGap;
    record.dstGap = dstGap;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcDataType = srcDataType;
    record.dstDataType = dstDataType;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::BROADCAST>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType srcDataType, DataType dstDataType>
AICORE_FUNC_HEAD void RecordBroadcastEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t config)
{
    const uint8_t nBurst = config & 0xff;
    const uint8_t lenBurst = (config >> 8U) & 0xff;
    const uint8_t srcGap = (config >> 16U) & 0xff;
    const uint8_t dstGap = (config >> 24U) & 0xff;
    bool enableRepeat = (config >> 63U) & 0x1;

    RecordBroadcastEvent<srcMemType, dstMemType, srcDataType, dstDataType>(EXTRA_PARAMS, dst, src, nBurst,
                                                            lenBurst, srcGap, dstGap, enableRepeat);
}

AICORE_FUNC_HEAD void RecordDcPreloadEvent(EXTRA_PARAMS_DEC, AddressSpace space, uint64_t src, int64_t offset)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    if (space == AddressSpace::PRIVATE) {
        auto head = reinterpret_cast<__gm__ RecordGlobalHead *>(memInfo);
        /// 不开启初始化检测时，过滤掉栈上的LOAD/STORE记录
        if (!head->checkParms.initcheck) {
            return;
        }
        src = StackAddrTransform(src);
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = DcPreloadRecord{};
    record.offset = offset;
    record.addr = GmAddrSubOffset(memInfo, MemType::GM, src);
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::DC_PRELOAD>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType, typename T>
AICORE_FUNC_HEAD void RecordLoadAWinograd(EXTRA_PARAMS_DEC,
                                        __ca__ T* dst, __cbuf__ T* src, uint16_t FMWidth, uint16_t FMHeight,
                                        uint16_t FMChannel, uint8_t dstGap, uint8_t colIndicator, uint8_t padModeHc,
                                        uint8_t padModeV, uint16_t stepK, uint16_t posK, uint16_t stepM, uint16_t posM)
{
    (void)padModeV;
    (void)colIndicator;
    (void)padModeHc;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadAWinogradRecord{};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.fmSizeW = FMWidth;
    record.fmSizeH = FMHeight;
    record.fmSizeCh = FMChannel;
    record.innerDstGap = dstGap;
    record.extStepK = stepK;
    record.extStepM = stepM;
    record.dstStartPointK = posK;
    record.dstStartPointM = posM;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.dataType = dataType;

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_A_WINOGRAD>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType, typename T>
AICORE_FUNC_HEAD void RecordLoadAWinograd(EXTRA_PARAMS_DEC,
                                        __ca__ T* dst, __cbuf__ T* src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadAWinogradRecord{};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.fmSizeW = config0 & 0xFFFF;
    record.fmSizeH = (config0 >> 16) & 0xFFFF;
    record.fmSizeCh = (config0 >> 32) & 0x0FFF;
    record.innerDstGap = (config0 >> 48) & 0x3F;
    record.extStepK = (config1 >> 8) & 0x0FFF;
    record.extStepM = (config1 >> 32) & 0xFFFF;
    record.dstStartPointK = (config1 >> 20) & 0x0FFF;
    record.dstStartPointM = (config1 >> 48) & 0x0FFF;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.dataType = dataType;

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_A_WINOGRAD>(record);
}

template<MemType srcMemType, MemType dstMemType, typename T>
AICORE_FUNC_HEAD void RecordLoadBWinograd(EXTRA_PARAMS_DEC,
                                        __cb__ T* dst, __cbuf__ T* src, uint8_t innerDstStride,
                                        uint16_t srcRepeatStride, uint8_t dstRepeatStride, uint8_t addr_SMASK,
                                        uint8_t weightIndicator, bool repeatIndicator, bool weightMatrixOffset,
                                        uint8_t repeatStride)
{
    (void)addr_SMASK;
    (void)weightIndicator;
    (void)repeatIndicator;
    (void)weightMatrixOffset;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadBWinogradRecord{};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.repeat = repeatStride;
    record.innerDstStride = innerDstStride;
    record.srcRptStride = srcRepeatStride;
    record.dstRptStride = dstRepeatStride;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_B_WINOGRAD>(record);
}

template<MemType srcMemType, MemType dstMemType, typename T>
AICORE_FUNC_HEAD void RecordLoadBWinograd(EXTRA_PARAMS_DEC, __cb__ T* dst, __cbuf__ T* src, uint64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = LoadBWinogradRecord{};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.repeat = (config >> 56) & 0xFF;
    record.innerDstStride = (config >> 8) & 0xFF;
    record.srcRptStride = (config >> 16) & 0xFFFF;
    record.dstRptStride = (config >> 32) & 0xFF;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::LOAD_B_WINOGRAD>(record);
}

AICORE_FUNC_HEAD void ParseLoad3DFMap2Config(Load3DRecord &record, uint64_t fmatrixConfig, uint64_t config1)
{
    record.fMapW = fmatrixConfig & 0xffff;
    record.fMapH = (fmatrixConfig >> 16U) & 0xffff;
    record.fMapC = (config1 >> 48U) & 0xffff;
    record.fMapLeftPad = (fmatrixConfig >> 32U) & 0xff;
    record.fMapRightPad = (fmatrixConfig >> 40U) & 0xff;
    record.fMapTopPad = (fmatrixConfig >> 48U) & 0xff;
    record.fMapBottomPad = (fmatrixConfig >> 56U) & 0xff;

    record.filterW = (((config1 >> 44U) & 0x1) << 8U) + ((config1 >> 12U) & 0xff);
    record.filterH = (((config1 >> 45U) & 0x1) << 8U) + ((config1 >> 20U) & 0xff);
    record.filterWStride = config1 & 0x3f;
    record.filterHStride = (config1 >> 6U) & 0x3f;
    record.filterWDilation = (config1 >> 28U) & 0xff;
    record.filterHDilation = (config1 >> 36U) & 0xff;
}

AICORE_FUNC_HEAD void ParseLoad3DMatrix2Config(Load3DRecord &record, uint64_t config0, uint64_t rpt)
{
    record.matrixKStep = config0 & 0xffff;
    record.matrixMStep = (config0 >> 16U) & 0xffff;
    record.matrixKPos = (config0 >> 32U) & 0xffff;
    record.matrixMPos = (config0 >> 48U) & 0xffff;
    if (rpt == 0) {
        record.matrixRptMode = 0;
        record.matrixRptStride = 0;
        record.matrixRptTimes = 1;
    } else {
        record.matrixRptMode = (rpt >> 24U) & 0x1;
        record.matrixRptStride = (rpt & 0xffff);
        record.matrixRptTimes = (rpt >> 16U) & 0xff;
    }
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoad3DEvent(EXTRA_PARAMS_DEC,
                                         uint64_t dst, uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    Load3DRecord record = Load3DRecord{};
    record.dst = dst;
    record.src = src;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;

    uint64_t fmatrixConfig = 0, rpt = 0;
    record.matrixMode = (config1 >> 47U) & 0x1;
    if (record.matrixMode == 0) {
        recorder.GetRegister(&Register::fmatrix, fmatrixConfig);
    } else {
        recorder.GetRegister(&Register::fmatrixB, fmatrixConfig);
    }
    recorder.GetRegister(&Register::l3dRpt, rpt);

    ParseLoad3DFMap2Config(record, fmatrixConfig, config1);
    ParseLoad3DMatrix2Config(record, config0, rpt);
    recorder.DumpRecord<RecordType::LOAD_3D>(record);
}

AICORE_FUNC_HEAD void ParseLoad3DV2FMap2Config(Load3DV2Record &record, uint64_t fmatrixConfig, uint64_t config1)
{
    record.fMapW = fmatrixConfig & 0xffff;
    record.fMapH = (fmatrixConfig >> 16U) & 0xffff;
    record.fMapC = (config1 >> 48U) & 0xffff;
    record.fMapLeftPad = (fmatrixConfig >> 32U) & 0xff;
    record.fMapRightPad = (fmatrixConfig >> 40U) & 0xff;
    record.fMapTopPad = (fmatrixConfig >> 48U) & 0xff;
    record.fMapBottomPad = (fmatrixConfig >> 56U) & 0xff;

    record.filterW = (((config1 >> 44U) & 0x1) << 8U) + ((config1 >> 12U) & 0xff);
    record.filterH = (((config1 >> 45U) & 0x1) << 8U) + ((config1 >> 20U) & 0xff);
    record.filterWStride = config1 & 0x3f;
    record.filterHStride = (config1 >> 6U) & 0x3f;
    record.filterWDilation = (config1 >> 28U) & 0xff;
    record.filterHDilation = (config1 >> 36U) & 0xff;

    record.transposeMode = (config1 >> 46) & 0x1;
}

AICORE_FUNC_HEAD void ParseLoad3DV2Matrix2Config(Load3DV2Record &record, uint64_t config0, uint64_t rpt)
{
    record.matrixKStep = config0 & 0xffff;
    record.matrixMStep = (config0 >> 16U) & 0xffff;
    record.matrixKPos = (config0 >> 32U) & 0xffff;
    record.matrixMPos = (config0 >> 48U) & 0xffff;

    record.matrixRptMode = (rpt >> 24U) & 0x1;
    record.matrixRptStride = (rpt & 0xffff);
    record.matrixRptTimes = (rpt >> 16U) & 0xff;
    record.dstStride = (rpt >> 32) & 0xff;
    record.outputMPos = (rpt >> 48) & 0xff;
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoad3DV2Event(EXTRA_PARAMS_DEC,
                                         uint64_t dst, uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    Load3DV2Record record = Load3DV2Record{};
    record.dst = dst;
    record.src = src & 0xffffffff;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;

    uint64_t fmatrixConfig = 0, rpt = 0;
    record.matrixMode = (config1 >> 47U) & 0x1;
    if (record.matrixMode == 0) {
        recorder.GetRegister(&Register::sprFmatrix, fmatrixConfig);
        recorder.GetRegister(&Register::sprL3dRpt, rpt);
    } else {
        recorder.GetRegister(&Register::sprFmatrixB, fmatrixConfig);
        recorder.GetRegister(&Register::sprL3dRptB, rpt);
    }

    ParseLoad3DV2FMap2Config(record, fmatrixConfig, config1);
    ParseLoad3DV2Matrix2Config(record, config0, rpt);
    recorder.DumpRecord<RecordType::LOAD_3D_V2>(record);
}

template<MemType dstMemType>
AICORE_FUNC_HEAD void RecordSet2DEvent(EXTRA_PARAMS_DEC, uint64_t dst, int64_t repeat)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    static_assert(dstMemType == MemType::L1 || dstMemType == MemType::L0A || dstMemType == MemType::L0B,
            "Unsupported memory types for SET_2D instruction.");
    uint64_t blockIdx = GetBlockIdx();
    auto record = Set2DRecord {};
    auto config = static_cast<uint64_t>(repeat);
    record.dst = dst;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.repeat = config & 0x7FFF;
    record.dstBlockNum = (config >> 16) & 0x7FFF;
    record.dstBlockSize = dstMemType == MemType::L1 ? 32 : 512;
    record.repeatGap = (config >> 32) & 0x7FFF;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::SET_2D>(record);
}

template<DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void RecordSetL12DEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = SetL12DRecord {};
    auto config = static_cast<uint64_t>(src);
    record.dst = dst;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.repeat = config & 0x7FFF;
    record.dstBlockNum = (config >> 16) & 0x7FFF;
    record.repeatGap = (config >> 32) & 0x7FFF;
    record.detailedDataType = detailedDataType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::SET_L1_2D>(record);
}

/// set_2d指令，A2训练对于L1，blockSize=32; 对于L0A/L0B，blockSize=512;
/// 对于A2推理芯片，对于L0A/L0B/L1，blockSize=512;
template<MemType dstMemType>
AICORE_FUNC_HEAD void RecordM200Set2DEvent(EXTRA_PARAMS_DEC, uint64_t dst, int64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    static_assert(dstMemType == MemType::L1 || dstMemType == MemType::L0A || dstMemType == MemType::L0B,
            "Unsupported memory types for SET_2D instruction.");
    uint64_t blockIdx = GetBlockIdx();
    Set2DRecord record{};
    uint8_t repeat = static_cast<uint64_t>(config) & 0xFF;
    record.dst = dst;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.repeat = 1;
    record.dstBlockNum = 1;
    record.dstBlockSize = 512 * repeat;
    record.repeatGap = 0;
    record.dstMemType = dstMemType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::SET_2D>(record);
}

template<MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoadImageEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                            uint16_t horSize, uint16_t verSize, uint16_t horStartP, uint16_t verStartP,
                                            uint16_t sHorRes, uint8_t topPadSize, uint8_t botPadSize, uint16_t lPadSize,
                                            uint16_t rPadSize)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    static_assert(dataType == DataType::DATA_B8 || dataType == DataType::DATA_B16,
            "These data types are not supported by LOAD_IMAGE instruction, it only supports B8 and B16");
    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    LoadImageRecord record = LoadImageRecord {};
    record.dst = dst;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.dstMemType = dstMemType;
    record.dataType = dataType;
    record.horSize = horSize;
    record.verSize = verSize;
    record.horStartP = horStartP;
    record.verStartP = verStartP;
    record.sHorRes = sHorRes;
    record.topPadSize = topPadSize;
    record.botPadSize = botPadSize;
    record.lPadSize = lPadSize;
    record.rPadSize = rPadSize;
    recorder.DumpRecord<RecordType::LOAD_IMAGE>(record);
}

template<MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordLoadImageEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t xs, uint64_t xt)
{
    uint8_t horSize = xs & 0x1FFF;
    uint16_t verSize = (xs >> 16) & 0x1FFF;
    uint16_t horStartP = (xs >> 32) & 0x1FFF;
    uint16_t verStartP = (xs >> 48) & 0x1FFF;
    uint16_t sHorRes = xt & 0xFFFF;
    uint8_t topPadSize = (xt >> 16) & 0xFF;
    uint8_t botPadSize = (xt >> 24) & 0xFF;
    uint16_t lPadSize = (xt >> 32) & 0x1FFF;
    uint16_t rPadSize = (xt >> 45) & 0x1FFF;
    RecordLoadImageEvent<dstMemType, dataType>(EXTRA_PARAMS, dst, horSize, verSize, horStartP, verStartP,
                                               sHorRes, topPadSize, botPadSize, lPadSize, rPadSize);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordMovAlignEvent(EXTRA_PARAMS_DEC,
                                           uint64_t dst, uint64_t src, uint8_t sid, uint16_t nBurst, uint32_t lenBurst,
                                           uint8_t leftPaddingNum, uint8_t rightPaddingNum,
                                           uint32_t srcGap, uint32_t dstGap)
{
    (void)sid;
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = MovAlignRecord{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.srcGap = srcGap;
    record.dstGap = dstGap;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.location.blockId = blockIdx;
    record.leftPaddingNum = leftPaddingNum;
    record.rightPaddingNum = rightPaddingNum;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.dataType = dataType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::MOV_ALIGN>(record);
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordMovAlignEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config, uint64_t gapConfig)
{
    uint8_t sid = config & 0xF;
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint32_t lenBurst = (config >> 16) & 0x1FFFFF;
    uint8_t leftPaddingNum = (config >> 48) & 0x3F;
    uint8_t rightPaddingNum = (config >> 54) & 0x3F;
    uint32_t srcGap = gapConfig & 0xFFFFFFFF;
    uint32_t dstGap = (gapConfig >> 32) & 0xFFFFFFFF;
    RecordMovAlignEvent<srcMemType, dstMemType, dataType>(EXTRA_PARAMS, dst, src, sid, nBurst, lenBurst,
                                                          leftPaddingNum, rightPaddingNum, srcGap, dstGap);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordMovAlignEventV2Stride(const Recorder &recorder, uint64_t config, uint64_t strideConfig,
                                                   MovAlignRecordV2 &record)
{
    uint64_t loop1Stride{};
    uint64_t loop2Stride{};
    uint64_t Register::* loop1StridePtr;
    uint64_t Register::* loop2StridePtr;
    if (dstMemType == MemType::GM) {
        loop1StridePtr = &Register::sprLoop1StrideUb2Out;
        loop2StridePtr = &Register::sprLoop2StrideUb2Out;
    } else if (dstMemType == MemType::UB) {
        loop1StridePtr = &Register::sprLoop1StrideOut2Ub;
        loop2StridePtr = &Register::sprLoop2StrideOut2Ub;
    } else {
        loop1StridePtr = &Register::sprLoop1StrideOut2L1;
        loop2StridePtr = &Register::sprLoop2StrideOut2L1;
    }
    recorder.GetRegister(loop1StridePtr, loop1Stride);
    recorder.GetRegister(loop2StridePtr, loop2Stride);

    if (dstMemType == MemType::GM) {
        // mov ub -> gm
        record.dstStride = GetUintFromConf<39, 0>(strideConfig);  // gm stride is always <39, 0>
        record.srcStride = GetUintFromConf<60, 40>(strideConfig); // ub stride is always <60, 40>
        record.loop1DstStride = GetUintFromConf<39, 0>(loop1Stride);
        record.loop1SrcStride = GetUintFromConf<60, 40>(loop1Stride);
        record.loop2DstStride = GetUintFromConf<39, 0>(loop2Stride);
        record.loop2SrcStride = GetUintFromConf<60, 40>(loop2Stride);
    } else {
        // mov gm -> ub or gm -> l1
        record.srcStride = GetUintFromConf<39, 0>(strideConfig);
        record.dstStride = GetUintFromConf<60, 40>(strideConfig);
        record.loop1SrcStride = GetUintFromConf<39, 0>(loop1Stride);
        record.loop1DstStride = GetUintFromConf<60, 40>(loop1Stride);
        record.loop2SrcStride = GetUintFromConf<39, 0>(loop2Stride);
        record.loop2DstStride = GetUintFromConf<60, 40>(loop2Stride);
        record.leftPaddingNum = GetUintFromConf<51, 46>(config);
        record.rightPaddingNum = GetUintFromConf<57, 52>(config);
    }
}

template<MemType srcMemType, MemType dstMemType, DataType dataType>
AICORE_FUNC_HEAD void RecordMovAlignEventV2(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config, uint64_t strideConfig)
{
    static_assert((srcMemType == MemType::UB && dstMemType == MemType::GM) ||
                  (srcMemType == MemType::GM && (dstMemType == MemType::UB || dstMemType == MemType::L1)),
                  "Unsupported memory type.");
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    uint64_t loopSize{};
    uint64_t Register::* loopSizePtr;
    if (dstMemType == MemType::GM) {
        loopSizePtr = &Register::sprLoopSizeUb2Out;
    } else if (dstMemType == MemType::UB) {
        loopSizePtr = &Register::sprLoopSizeOut2Ub;
    } else {
        loopSizePtr = &Register::sprLoopSizeOut2L1;
    }
    recorder.GetRegister(loopSizePtr, loopSize);

    MovAlignRecordV2 record{};
    record.dst = GmAddrSubOffset(memInfo, dstMemType, dst);
    record.src = GmAddrSubOffset(memInfo, srcMemType, src);
    record.nBurst = GetUintFromConf<24, 4>(config);
    record.lenBurst = GetUintFromConf<45, 25>(config);
    record.loop1Size = GetUintFromConf<20, 0>(loopSize);
    record.loop2Size = GetUintFromConf<42, 21>(loopSize);
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.dataType = dataType;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    RecordMovAlignEventV2Stride<srcMemType, dstMemType>(recorder, config, strideConfig, record);
    recorder.DumpRecord<RecordType::MOV_ALIGN_V2>(record);
}

template<DataType dataType>
AICORE_FUNC_HEAD void RecordNdDMAOut2Ub(EXTRA_PARAMS_DEC, uint64_t dst,
                                           uint64_t src, uint64_t config0, uint64_t config1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    NdDMAOut2UbRecord record{};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dst = dst;
    record.src = GmAddrSubOffset(memInfo, MemType::GM, src);
    record.dataType = dataType;
    uint64_t sprPadCntNdDma{};
    uint64_t sprLoopStrideNdDma[5];
    recorder.GetRegister(&Register::sprPadCntNdDma, sprPadCntNdDma);
    recorder.GetRegister(&Register::sprLoop0StrideNdDma, sprLoopStrideNdDma[0]);
    recorder.GetRegister(&Register::sprLoop1StrideNdDma, sprLoopStrideNdDma[1]);
    recorder.GetRegister(&Register::sprLoop2StrideNdDma, sprLoopStrideNdDma[2]);
    recorder.GetRegister(&Register::sprLoop3StrideNdDma, sprLoopStrideNdDma[3]);
    recorder.GetRegister(&Register::sprLoop4StrideNdDma, sprLoopStrideNdDma[4]);
    record.loop[0].loopSize = GetUintFromConf<23, 4>(config0);
    record.loop[1].loopSize = GetUintFromConf<43, 24>(config0);
    record.loop[2].loopSize = GetUintFromConf<63, 44>(config0);
    record.loop[3].loopSize = GetUintFromConf<19, 0>(config1);
    record.loop[4].loopSize = GetUintFromConf<39, 20>(config1);
    record.loop[0].loopLpSize = GetUintFromConf<47, 40>(config1);
    record.loop[0].loopRpSize = GetUintFromConf<55, 48>(config1);
    record.loop[1].loopLpSize = GetUintFromConf<7, 0>(sprPadCntNdDma);
    record.loop[1].loopRpSize = GetUintFromConf<15, 8>(sprPadCntNdDma);
    record.loop[2].loopLpSize = GetUintFromConf<23, 16>(sprPadCntNdDma);
    record.loop[2].loopRpSize = GetUintFromConf<31, 24>(sprPadCntNdDma);
    record.loop[3].loopLpSize = GetUintFromConf<39, 32>(sprPadCntNdDma);
    record.loop[3].loopRpSize = GetUintFromConf<47, 40>(sprPadCntNdDma);
    record.loop[4].loopLpSize = GetUintFromConf<55, 48>(sprPadCntNdDma);
    record.loop[4].loopRpSize = GetUintFromConf<63, 56>(sprPadCntNdDma);
    for (size_t i = 0; i < NdDMAOut2UbRecord::LOOP; ++i) {
        record.loop[i].loopDstStride = GetUintFromConf<19, 0>(sprLoopStrideNdDma[i]);
        record.loop[i].loopSrcStride = GetUintFromConf<59, 20>(sprLoopStrideNdDma[i]);
    }
    recorder.DumpRecord<RecordType::ND_DMA_OUT_TO_UB>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordMovBtEvent(EXTRA_PARAMS_DEC,
                                          uint64_t dst, uint64_t src, uint16_t convControl, uint16_t nBurst,
                                          uint16_t lenBurst, uint16_t sourceGap, uint16_t dstGap)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = MovBtRecord {};
    record.dst = dst;
    record.src = src;
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.srcGap = sourceGap;
    record.dstGap = dstGap;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.convControl = convControl;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::MOV_BT>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordMovBtEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t config)
{
    uint16_t convControl = (config >> 3) & 0x1;
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcGap = (config >> 32) & 0xFFFF;
    uint16_t dstGap = (config >> 48) & 0xFFFF;

    RecordMovBtEvent<srcMemType, dstMemType>(EXTRA_PARAMS, dst, src, convControl, nBurst, lenBurst,
                                             srcGap, dstGap);
}

AICORE_FUNC_HEAD bool IsMovFpQuantToB16(uint64_t quantPRE)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    // 950: 1,10,11,16,31,32,33,34
    return (quantPRE == 1 || quantPRE == 10 || quantPRE == 11 || quantPRE == 16 || (quantPRE >= 31 && quantPRE <= 34));
#else
    // 910b: 1,6,10,11,12,13,16
    return (quantPRE == 1 || quantPRE == 6 || (quantPRE >= 10 && quantPRE <= 13) || quantPRE == 16);
#endif
}

AICORE_FUNC_HEAD bool IsMovFpQuantToB8(uint64_t quantPRE)
{
#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)
    // 950: 2,3,4,5,8,9,12,13,23,24
    return ((quantPRE >= 2 && quantPRE <= 5) || quantPRE == 8 || quantPRE == 9 ||
        quantPRE == 12 || quantPRE == 13 || quantPRE == 23 || quantPRE == 24);
#else
    // 910b: 8,9,23,24
    return (quantPRE == 8 || quantPRE == 9 || quantPRE == 23 || quantPRE == 24);
#endif
}

AICORE_FUNC_HEAD bool IsMovFpQuantToB4(uint64_t quantPRE)
{
    // 950/910b: 21,22,25,26
    return (quantPRE == 21 || quantPRE == 22 || quantPRE == 25 || quantPRE == 26);
}

AICORE_FUNC_HEAD void ParseMovFpQuantBits(uint64_t quantPRE, bool enNDorDN, MovFpRecord& record)
{
    if (IsMovFpQuantToB16(quantPRE)) {
        record.quantPreBits = 16;  // f16/s16/bf16
    } else if (IsMovFpQuantToB8(quantPRE)) {
        record.int8ChannelMerge = enNDorDN ? false : true;
        record.quantPreBits = 8;  // s8/u8/HiF8
    } else if (IsMovFpQuantToB4(quantPRE)) {
        record.int4ChannelMerge = enNDorDN ? false : true;
        record.quantPreBits = 4;  // s4
    } else {
        record.quantPreBits = 32;  // f32/s32
    }
}

AICORE_FUNC_HEAD void RecordMovFpEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint16_t nSize,
                                        uint16_t mSize, uint32_t dstStrideDstD, uint16_t srcStride,
                                        uint8_t unitFlag, uint64_t quantPRE, bool channelSplit,
                                        bool enNZ2ND, bool isDstF32)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    auto record = MovFpRecord {};
    record.isC310 = false;
    record.dst = GmAddrSubOffset(memInfo, MemType::GM, dst);
    record.src = src;
    record.nSize = nSize;
    record.mSize = mSize;
    record.dstStride = dstStrideDstD; // enNZ2ND=true: in unit of element; enNZ2ND=false: in unit of 32B
    record.srcStride = srcStride; // in unit of C0 SIZE
    record.enUnitFlag = (unitFlag > 1);  // Mode2/Mode3表示使能unit-flag机制

    // 判断属于哪种模式
    ParseMovFpQuantBits(quantPRE, enNZ2ND, record);
    if (isDstF32 && channelSplit && !enNZ2ND) {
        record.channelSplit = true;
    } else {
        record.channelSplit = false;
    }
    record.enNZ2ND = enNZ2ND;
    record.enNZ2DN = false;

    uint64_t blockIdx = GetBlockIdx();
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    // ndNum/srcNdStride/dstNdStride来自ND_PARA，从桩函数set_nd_para中获取
    uint64_t ndParaConfig = 0;
    recorder.GetRegister(&Register::ndParaConfig, ndParaConfig);
    record.ndNum = ndParaConfig & 0xFFFF;
    record.srcNdStride = (ndParaConfig >> 16) & 0xFFFF; // in unit of fractal size(1024B)
    record.dstNdStride = (ndParaConfig >> 32) & 0xFFFF; // in unit of element
    recorder.DumpRecord<RecordType::MOV_FP>(record);
}

AICORE_FUNC_HEAD void RecordMovFpEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                        uint64_t xm, uint64_t xt, bool isDstF32)
{
    uint16_t nSize = (xm >> 4) & 0xFFF;
    uint16_t mSize = (xm >> 16) & 0xFFFF;
    uint32_t dstStrideDstD = (xm >> 32) & 0xFFFFFFFF;
    uint16_t srcStride = xt & 0xFFFF;
    uint8_t unitFlag = (xt >> 32) & 0x3;
    uint64_t quantPRE = (xt >> 34) & 0x1F;
    bool channelSplit = (xt >> 42) & 0x1;
    bool enNZ2ND = (xt >> 43) & 0x1;

    RecordMovFpEvent(EXTRA_PARAMS, dst, src, nSize, mSize, dstStrideDstD,
                     srcStride, unitFlag, quantPRE, channelSplit, enNZ2ND, isDstF32);
}

template <RecordType recordType>
AICORE_FUNC_HEAD void RecordMovFpV2Event(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                          uint64_t xm, uint64_t xt, bool isDstF32)
{
    if (InvalidMemInfo(memInfo)) { return;}

    auto record = MovFpRecord {};
    record.isC310 = true; // c310的读写行为和910B不同，需要区分
    record.dst = GmAddrSubOffset(memInfo, MemType::GM, dst);
    record.src = src;
    record.nSize = (xm >> 4) & 0xFFF;
    record.mSize = (xm >> 16) & 0xFFFF;
    record.dstStride = (xm >> 32) & 0xFFFFFFFF; // in unit of element
    record.srcStride = xt & 0xFFFF; // in unit of C0 SIZE
    uint8_t unitFlag = (xt >> 32) & 0x3;
    record.enUnitFlag = (unitFlag > 1); // Mode2/Mode3表示使能unit-flag机制
    record.dualDstMode = (xt >> 16) & 0x3; // dual destination mode: 0=single, 1=split M, 2=split N
    record.subVecBlockId = (xt >> 18) & 0x1;
    // 判断属于哪种模式
    uint64_t bit29 = (xt >> 29) & 0x1;
    uint64_t bits34_38 = (xt >> 34) & 0x1F;
    uint64_t quantPRE = (bit29 << 5) | bits34_38; // quantPRE由Xt[29]和Xt[38:34]组合成6-bit
    record.enNZ2ND = (xt >> 43) & 0x1;
    record.enNZ2DN = (xt >> 62) & 0x1;
    bool enNDorDN = record.enNZ2ND || record.enNZ2DN;
    ParseMovFpQuantBits(quantPRE, enNDorDN, record);
    bool channelSplit = (xt >> 42) & 0x1;
    if (isDstF32 && channelSplit && !enNDorDN) {
        record.channelSplit = true;
    } else {
        record.channelSplit = false;
    }

    uint64_t blockIdx = GetBlockIdx();
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    // ndNum/srcNdStride/dstNdStride来自LOOP3_PARA，从桩函数set_loop3_para中获取
    uint64_t sprLoop3Para = 0;
    recorder.GetRegister(&Register::sprLoop3Para, sprLoop3Para);
    record.ndNum = sprLoop3Para & 0xFFFF;
    record.srcNdStride = (sprLoop3Para >> 16) & 0xFFFF; // in unit of C0 SIZE
    record.dstNdStride = (sprLoop3Para >> 32) & 0xFFFFFFFF; // in unit of element

    if (record.enNZ2DN) {
        // enNZ2DN使能时，srcNzC0Stride(Loop0_src_stride)从桩函数set_channel_para中获取
        uint64_t sprChannelPara = 0;
        recorder.GetRegister(&Register::sprChannelPara, sprChannelPara);
        record.srcNzC0Stride = (sprChannelPara >> 48) & 0xFFFF; // in unit of C0 SIZE
        if (record.srcNzC0Stride != 1) {
            // When NZ2DN is enable and loop0 src stride is not 1, unit-flag cannot be enable.
            record.enUnitFlag = false;
        }
    }
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void RecordMovFpV2Event(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                          uint64_t xm, uint64_t xt, bool isDstF32)
{
    RecordMovFpV2Event<RecordType::MOV_FP>(EXTRA_PARAMS, dst, src, xm, xt, isDstF32);
}

AICORE_FUNC_HEAD void RecordFixL0CToL1Event(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                             uint64_t xm, uint64_t xt, bool isDstF32)
{
    RecordMovFpV2Event<RecordType::FIX_L0C_TO_L1>(EXTRA_PARAMS, dst, src, xm, xt, isDstF32);
    UpdateLreluAlpha(EXTRA_PARAMS, isDstF32);
}

AICORE_FUNC_HEAD void RecordFixL0CToUBEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src,
                                             uint64_t xm, uint64_t xt, bool isDstF32)
{
    RecordMovFpV2Event<RecordType::FIX_L0C_TO_UB>(EXTRA_PARAMS, dst, src, xm, xt, isDstF32);
}

template <MemType srcMemType, MemType dstMemType, DataType dataType,  typename T>
AICORE_FUNC_HEAD void RecordLoadB2Event(EXTRA_PARAMS_DEC, __cb__ T *dst, __cbuf__ T *src, uint8_t repeat)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockID = GetBlockIdx();
    auto record = LoadB2Record{};
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.repeat = repeat;
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.location.blockId = blockID;
    record.dataType = dataType;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    Recorder recorder(memInfo, blockID);
    recorder.DumpRecord<RecordType::LOAD_B2>(record);
}

template <MemType srcMemType, MemType dstMemType, DataType dataType, typename T>
AICORE_FUNC_HEAD void RecordLoadB2Event(EXTRA_PARAMS_DEC, __cb__ T *dst, __cbuf__ T *src, uint64_t config)
{
    if (!memInfo) {
        return;
    }
    uint8_t repeat = (config >> 16) & 0xFF;
    RecordLoadB2Event<srcMemType, dstMemType, dataType>(EXTRA_PARAMS, dst, src, repeat);
}

template<RecordType recordType, DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void RecordScalarRedAndAtomEvent(EXTRA_PARAMS_DEC, uint64_t addr, uint64_t size)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = RedRecord{};
    record.addr = addr;
    record.size = size;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.isAtom = recordType == RecordType::SCALAR_ATOM;
    record.detailedDataType = detailedDataType;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<MemType srcMemType, MemType dstMemType, RecordType recordType>
AICORE_FUNC_HEAD void RecordDmaMovL1OrUbEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint16_t nBurst, uint16_t lenBurst,
                                         uint16_t srcGap, uint16_t dstGap)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    auto record = MovL1UBRecord{};
    record.dst = dst;
    record.src = src;
    record.nBurst = nBurst;
    record.lenBurst = lenBurst;
    record.srcGap = srcGap;
    record.dstGap = dstGap;
    record.srcMemType = srcMemType;
    record.dstMemType = dstMemType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordDmaMovL2UBEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config)
{
    uint8_t ubDestId = config & 0x1;
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcGap = (config >> 32) & 0xFFFF;
    uint16_t dstGap = (config >> 48) & 0xFFFF;

    RecordDmaMovL1OrUbEvent<srcMemType, dstMemType, RecordType::MOV_L1_TO_UB>(EXTRA_PARAMS, dst, src, nBurst, lenBurst, srcGap,
                                              dstGap);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordDmaMovUB2L1Event(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config)
{
    uint16_t nBurst = (config >> 4) & 0xFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcGap = (config >> 32) & 0xFFFF;
    uint16_t dstGap = (config >> 48) & 0xFFFF;

    RecordDmaMovL1OrUbEvent<srcMemType, dstMemType, RecordType::MOV_UB_TO_L1>(EXTRA_PARAMS, dst, src, nBurst, lenBurst, srcGap,
                                              dstGap);
}

template<MemType srcMemType, MemType dstMemType>
AICORE_FUNC_HEAD void RecordDmaMovUB2UBEvent(EXTRA_PARAMS_DEC, uint64_t dst,
                                         uint64_t src, uint64_t config)
{
    uint16_t nBurst = config & 0xFFFF;
    uint16_t lenBurst = (config >> 16) & 0xFFFF;
    uint16_t srcGap = (config >> 32) & 0xFFFF;
    uint16_t dstGap = (config >> 48) & 0xFFFF;

    RecordDmaMovL1OrUbEvent<srcMemType, dstMemType, RecordType::MOV_UB_TO_UB>(EXTRA_PARAMS, dst, src, nBurst, lenBurst, srcGap,
                                              dstGap);
}

template<DetailedDataType dataType>
AICORE_FUNC_HEAD void RecordDmaMovL1BtEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    MovL1BtRecord record{};
    record.dst = dst;
    record.src = src;
    record.nBurst = GetUintFromConf<15, 4>(config);
    record.lenBurst = GetUintFromConf<31, 16>(config);
    record.srcGap = GetUintFromConf<47, 32>(config);
    record.dstGap = GetUintFromConf<63, 48>(config);
    record.dataType = dataType;
    record.cvtEnable = GetUintFromConf<3, 3>(config);

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::MOV_CBUF_TO_BT>(record);
}

AICORE_FUNC_HEAD void RecordDmaMovL1FbEvent(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    MovL1FbRecord record{};
    record.dst = GetUintFromConf<15, 0>(dst);
    record.src = src;
    record.nBurst = GetUintFromConf<15, 4>(config);
    record.lenBurst = GetUintFromConf<31, 16>(config);
    record.srcStride = GetUintFromConf<47, 32>(config);
    record.dstStride = GetUintFromConf<63, 48>(config);
    record.dstMemBlock = GetUintFromConf<19, 16>(dst);

#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;

    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::MOV_CBUF_TO_FB>(record);
}

}  // namespace Sanitizer

#endif  // PLUGIN_RECORD_MOVE_INSTRUCTIONS_H
