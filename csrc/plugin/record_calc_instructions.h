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

#ifndef PLUGIN_RECORD_CALC_INSTRUCTIONS_H
#define PLUGIN_RECORD_CALC_INSTRUCTIONS_H

#include <utility>

#include "plugin/utils.h"
#include "recorder.h"
#include "utils.h"
#include "core/framework/constant.h"

namespace Sanitizer {

template<typename T>
struct DataBitSize {
    static constexpr uint64_t value = sizeof(T) * BITS_EACH_BYTE;
};

template<>
struct DataBitSize<void> {
    static constexpr uint64_t value = BITS_EACH_BYTE;
};

template <typename Record>
AICORE_FUNC_HEAD void ParseVectorMask(Record &record, Recorder const &recorder, bool useMask)
{
    if (useMask) {
        recorder.GetRegister(&Register::maskMode, record.maskMode);
        recorder.GetRegister(&Register::vectorMask0, record.vectorMask.mask0);
        recorder.GetRegister(&Register::vectorMask1, record.vectorMask.mask1);
    } else {
        record.maskMode = MaskMode::MASK_NORM;
        record.vectorMask.mask0 = static_cast<uint64_t>(-1);
        record.vectorMask.mask1 = static_cast<uint64_t>(-1);
    }
}

template <typename T>
AICORE_FUNC_HEAD void ParseCompareMask(CompareMask &compareMask, __ubuf__ T *ubuf)
{
    // compare mask 以 128 bits 的方式保存在 UB 上，和 vector mask 保持一致，
    // 高位保存在 mask1 中，低位保存在 mask0 中
    auto mask = static_cast<__ubuf__ uint64_t *>(static_cast<__ubuf__ void *>(ubuf));
    // 只有 b16 数据才需要用到 128 位 mask，b32 数据只需要使用 64 位
    if (sizeof(T) == 2U) {
        compareMask.mask0 = mask[1];
        compareMask.mask1 = mask[0];
    } else {
        compareMask.mask0 = mask[0];
    }
}

template<RecordType recordType, typename TO, typename TI>
AICORE_FUNC_HEAD void ParseDataBits(UnaryOpRecord &record)
{
    if (recordType == RecordType::VCONV_DST_S4_OP) {
        constexpr uint8_t dstS4DataBits = 4U;
        record.dstDataBits = dstS4DataBits;
        record.srcDataBits = DataBitSize<TI>::value;
    } else if (recordType == RecordType::VCONV_SRC_S4_OP) {
        constexpr uint8_t srcS4DataBits = 4U;
        record.dstDataBits = DataBitSize<TO>::value;
        record.srcDataBits = srcS4DataBits;
    } else {
        record.dstDataBits = DataBitSize<TO>::value;
        record.srcDataBits = DataBitSize<TI>::value;
    }
}

template<RecordType recordType, typename TO, typename TI0, typename TI1>
AICORE_FUNC_HEAD void ParseDataBits(BinaryOpRecord &record)
{
    if (recordType == RecordType::VSEL_OP) {
        record.dstDataBits = DataBitSize<TO>::value;
        record.src0DataBits = DataBitSize<TI0>::value;
        record.src1DataBits = DataBitSize<TI0>::value;
    } else {
        record.dstDataBits = DataBitSize<TO>::value;
        record.src0DataBits = DataBitSize<TI0>::value;
        record.src1DataBits = DataBitSize<TI1>::value;
    }
}

AICORE_FUNC_HEAD void ParseVaRegister(Recorder const &recorder, VaRegister &vaRegister0,
                                       VaRegister &vaRegister1, ub_addr8_t addr)
{
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1
    switch (addr) {
        case VA0: {
            recorder.GetRegister(&Register::va0, vaRegister0);
            recorder.GetRegister(&Register::va1, vaRegister1);
            break;
        }
        case VA2: {
            recorder.GetRegister(&Register::va2, vaRegister0);
            recorder.GetRegister(&Register::va3, vaRegister1);
            break;
        }
        case VA4: {
            recorder.GetRegister(&Register::va4, vaRegister0);
            recorder.GetRegister(&Register::va5, vaRegister1);
            break;
        }
        case VA6: {
            recorder.GetRegister(&Register::va6, vaRegister0);
            recorder.GetRegister(&Register::va7, vaRegister1);
            break;
        }
        default:
            return;
    }
#endif
}

template<RecordType recordType, typename TO, typename TI>
AICORE_FUNC_HEAD void RecordUnaryOpFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI *src, uint8_t repeat, uint16_t dstBlockStride, uint16_t srcBlockStride,
    uint16_t dstRepeatStride, uint16_t srcRepeatStride, uint8_t dstBlockNum, uint8_t srcBlockNum,
    uint64_t dstBlockSize, uint64_t srcBlockSize, bool useMask = true)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = UnaryOpRecord {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src =  reinterpret_cast<uint64_t>(src);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dstBlockStride = dstBlockStride;
    record.srcBlockStride = srcBlockStride;
    record.dstRepeatStride = dstRepeatStride;
    record.srcRepeatStride = srcRepeatStride;
    record.repeat = repeat;
    record.dstBlockNum = dstBlockNum;
    record.srcBlockNum = srcBlockNum;
    record.dstBlockSize = dstBlockSize;
    record.srcBlockSize = srcBlockSize;
    ParseDataBits<recordType, TO, TI>(record);
    Recorder recorder(memInfo, blockIdx);
    ParseVectorMask(record, recorder, useMask);
    recorder.DumpRecord<recordType>(record);
}

template <typename T,
          typename = typename std::enable_if<std::is_same<T, half>::value || std::is_same<T, int16_t>::value ||
                                             std::is_same<T, uint16_t>::value || std::is_same<T, __bf16>::value ||
                                             std::is_same<T, float>::value || std::is_same<T, uint32_t>::value ||
                                             std::is_same<T, int32_t>::value>::type>
AICORE_FUNC_HEAD void RecordVecDupEvent(EXTRA_PARAMS_DEC, __ubuf__ T *dst,
                                         uint8_t repeat, uint16_t dstBlockStride, uint16_t dstRepeatStride)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    constexpr uint8_t dataBitsForB16 = 16;
    constexpr uint8_t dataBitsForB32 = 32;
    uint64_t blockIdx = GetBlockIdx();
    auto record = VecDupRecord{};
    /// vector_dup指令，对于b16类型数据，取dst低16位地址；对于b32数据，取dst低32位地址
    if (sizeof(T) == 2) {
        record.dst = reinterpret_cast<uint64_t>(dst) & 0x0000FFFF;
        record.dataBits = dataBitsForB16;
    } else {
        record.dst = reinterpret_cast<uint64_t>(dst) & 0xFFFFFFFF;
        record.dataBits = dataBitsForB32;
    }
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);

    record.repeat = repeat;
    record.dstBlockStride = dstBlockStride;
    record.dstRepeatStride = dstRepeatStride;

    Recorder recorder(memInfo, blockIdx);
    ParseVectorMask(record, recorder, true);
    recorder.DumpRecord<RecordType::VEC_DUP>(record);
}

template <typename T>
AICORE_FUNC_HEAD void RecordVecDupEvent(EXTRA_PARAMS_DEC, __ubuf__ T *dst, uint64_t config)
{
    uint8_t repeat = (config >> 56) & 0xFF;
    uint16_t dstBlockStride = config & 0xFFFF;
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    uint16_t dstRepeatStride = ((config >> 32) & 0xFF) | ((config >> 44) & 0xF00);
#else
    uint16_t dstRepeatStride = ((config >> 32) & 0xFF);
#endif
    RecordVecDupEvent(EXTRA_PARAMS, dst, repeat, dstBlockStride, dstRepeatStride);
}

template<RecordType recordType, typename TO, typename TI0, typename TI1>
AICORE_FUNC_HEAD void RecordBinaryOpFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI0 *src0, __ubuf__ TI1 *src1, uint8_t repeat, uint16_t dstBlockStride,
    uint16_t src0BlockStride, uint16_t src1BlockStride, uint16_t dstRepeatStride, uint16_t src0RepeatStride,
    uint16_t src1RepeatStride, uint8_t dstBlockNum, uint8_t src0BlockNum, uint8_t src1BlockNum,
    uint16_t dstBlockSize, uint16_t src0BlockSize, uint16_t src1BlockSize, bool useMask = true)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = BinaryOpRecord {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src0 = reinterpret_cast<uint64_t>(src0);
    record.src1 = reinterpret_cast<uint64_t>(src1);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dstBlockStride = dstBlockStride;
    record.src0BlockStride = src0BlockStride;
    record.src1BlockStride = src1BlockStride;
    record.dstRepeatStride = dstRepeatStride;
    record.src0RepeatStride = src0RepeatStride;
    record.src1RepeatStride = src1RepeatStride;
    record.repeat = repeat;
    record.dstBlockNum = dstBlockNum;
    record.src0BlockNum = src0BlockNum;
    record.src1BlockNum = src1BlockNum;
    record.dstBlockSize = dstBlockSize;
    record.src0BlockSize = src0BlockSize;
    record.src1BlockSize = src1BlockSize;
    ParseDataBits<recordType, TO, TI0, TI1>(record);
    Recorder recorder(memInfo, blockIdx);
    ParseVectorMask(record, recorder, useMask);
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void RecordVgatherbConfigFunc(uint64_t config, uint32_t &offsetAddr, uint16_t &dstRepeatStride,
                                                uint8_t &dstBlockStride, uint8_t &repeat)
{
    offsetAddr = config & 0xFFFFFFFF;
    auto lowDstRepeatStride = (config >> 44U) & 0xF00;
    dstRepeatStride = static_cast<uint16_t>(lowDstRepeatStride) | ((config >> 32U) & 0xFF);
    dstBlockStride = (config >> 40U) & 0xFF;
    repeat = (config >> 56U) & 0xFF;
}

template<RecordType recordType, typename TO, typename TI>
AICORE_FUNC_HEAD void RecordReduceOpFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI *src, uint8_t repeat, uint16_t dstRepeatStride, uint16_t srcBlockStride,
    uint16_t srcRepeatStride, uint16_t dstRepeatLength, uint8_t dstBlockNum, uint8_t srcBlockNum,
    uint8_t dstBlockSize, uint8_t srcBlockSize, uint16_t dstAlignSize = 32, uint8_t dstDataBitsFactor = 1)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = ReduceOpRecord {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dstRepeatStride = dstRepeatStride;
    record.srcBlockStride = srcBlockStride;
    record.srcRepeatStride = srcRepeatStride;
    record.dstRepeatLength = dstRepeatLength;
    record.dstAlignSize = dstAlignSize;
    record.repeat = repeat;
    record.dstBlockNum = dstBlockNum;
    record.srcBlockNum = srcBlockNum;
    record.dstBlockSize = dstBlockSize;
    record.srcBlockSize = srcBlockSize;
    record.dstDataBits = DataBitSize<TO>::value * dstDataBitsFactor;
    record.srcDataBits = DataBitSize<TI>::value;
    record.dstDataBitsFactor = dstDataBitsFactor;
    Recorder recorder(memInfo, blockIdx);
    // 所有归约类指令都支持 vector mask
    ParseVectorMask(record, recorder, true);
    recorder.DumpRecord<recordType>(record);
}

AICORE_FUNC_HEAD void ParseVecRegProCoordConfig(uint64_t config, uint8_t& repeat, uint8_t& regionRange)
{
    constexpr uint8_t repeatShift = 56;
    constexpr uint8_t regionRangeShift = 16;

    constexpr uint8_t repeatWidth = 8;
    constexpr uint8_t regionRangeWidth = 3;

    repeat = GetIntFromConf<repeatShift, repeatWidth>(config);
    regionRange = GetIntFromConf<regionRangeShift, regionRangeWidth>(config);
}

template <RecordType recordType, DataType dataType, typename T>
AICORE_FUNC_HEAD void RecordVRegPropCoorOpFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src, uint8_t repeat,
                                                uint8_t regionRange, bool isExtract)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = VecRegPropCoordOpRecord {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
    record.location.blockId = blockIdx;
    record.dataType = dataType;
    record.repeat = repeat;
    record.regionRange = regionRange;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.isExtract = isExtract;
    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template <RecordType recordType, DataType dataType, typename T>
AICORE_FUNC_HEAD void RecordVRegPropCoorOpFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src,
                                                uint64_t config, bool isExtract)
{
    uint8_t repeat = 0;
    uint8_t regionRange = 0;
    ParseVecRegProCoordConfig(config, repeat, regionRange);
    RecordVRegPropCoorOpFunc<recordType, dataType, T>(EXTRA_PARAMS, dst, src, repeat, regionRange, isExtract);
}

AICORE_FUNC_HEAD void ParseBinaryConfig(uint64_t config, uint8_t &repeat, uint8_t &dstBlockStride,
    uint8_t &src0BlockStride, uint8_t &src1BlockStride, uint8_t &dstRepeatStride, uint8_t &src0RepeatStride,
    uint8_t &src1RepeatStride)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t src0BlockStrideShift = 8;
    constexpr uint64_t src1BlockStrideShift = 16;
    constexpr uint64_t dstRepeatStrideShift = 24;
    constexpr uint64_t src0RepeatStrideShift = 32;
    constexpr uint64_t src1RepeatStrideShift = 40;

    repeat = (config >> repeatShift) & 0xFF;
    dstBlockStride = config & 0xFF;
    src0BlockStride = (config >> src0BlockStrideShift) & 0xFF;
    src1BlockStride = (config >> src1BlockStrideShift) & 0xFF;
    dstRepeatStride = (config >> dstRepeatStrideShift) & 0xFF;
    src0RepeatStride = (config >> src0RepeatStrideShift) & 0xFF;
    src1RepeatStride = (config >> src1RepeatStrideShift) & 0xFF;
}

AICORE_FUNC_HEAD void ParseVredecev2Config(uint64_t config, uint16_t &repeat, uint8_t &src0BlockStride,
    uint8_t &patternMode, uint16_t &src0RepeatStride, uint16_t &src1RepeatStride)
{
    constexpr uint64_t highRepeatShift = 56;
    constexpr uint64_t src0BlockStrideShift = 8;
    constexpr uint64_t patternModeShift = 16;
    constexpr uint64_t src0RepeatStrideShift = 24;
    constexpr uint64_t src1RepeatStrideShift = 40;
    auto highRepeat = (config >> highRepeatShift) & 0xFF;
    repeat = (static_cast<uint16_t>(highRepeat) << 8U) + (config & 0XFF);
    src0BlockStride = (config >> src0BlockStrideShift) & 0xFF;
    patternMode = (config >> patternModeShift) & 0xFF;
    src0RepeatStride = (config >> src0RepeatStrideShift) & 0xFFFF;
    src1RepeatStride = (config >> src1RepeatStrideShift) & 0xFFFF;
}

template <typename T,
    typename = typename std::enable_if<
    std::is_same<T, uint8_t>::value ||
    std::is_same<T, uint16_t>::value >::type
>
AICORE_FUNC_HEAD void ParseUnaryConfig(uint64_t config, uint8_t &repeat, uint16_t &dstBlockStride,
    uint16_t &srcBlockStride, T &dstRepeatStride, T &srcRepeatStride)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t srcBlockStrideShift = 16;
    constexpr uint64_t dstRepeatStrideShift1 = 32;
    constexpr uint64_t srcRepeatStrideShift = 40;

    repeat = (config >> repeatShift) & 0xFF;
    dstBlockStride = config & 0xFFFF;
    srcBlockStride = (config >> srcBlockStrideShift) & 0xFFFF;

    if (sizeof(T) == sizeof(uint8_t)) {
        dstRepeatStride = (config >> dstRepeatStrideShift1) & 0xFF;
        srcRepeatStride = (config >> srcRepeatStrideShift) & 0xFF;
    } else {
        constexpr uint64_t dstRepeatStrideShift2 = 44;
        dstRepeatStride = ((config >> dstRepeatStrideShift1) & 0xFF) | ((config >> dstRepeatStrideShift2) & 0xF00);
        srcRepeatStride = (config >> srcRepeatStrideShift) & 0xFFF;
    }
}

AICORE_FUNC_HEAD void ParseUnaryConfigByArch(uint64_t config, uint8_t &repeat, uint16_t &dstBlockStride,
    uint16_t &srcBlockStride, uint16_t &dstRepeatStride, uint16_t &srcRepeatStride)
{
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__)
    ParseUnaryConfig<uint16_t>(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
#else
    auto dstRepeatStrideU8 = uint8_t{};
    auto srcRepeatStrideU8 = uint8_t{};
    ParseUnaryConfig<uint8_t>(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStrideU8, srcRepeatStrideU8);
    dstRepeatStride = dstRepeatStrideU8;
    srcRepeatStride = srcRepeatStrideU8;
#endif
}

AICORE_FUNC_HEAD void ParseReduceConfig(uint64_t config, uint8_t &repeat, uint16_t &dstRepeatStride,
    uint16_t &srcBlockStride, uint16_t &srcRepeatStride)
{
    constexpr uint64_t repeatShift = 56;
    constexpr uint64_t srcBlockStrideShift = 16;
    constexpr uint64_t srcRepeatStrideShift = 32;

    repeat = (config >> repeatShift) & 0xFF;
    dstRepeatStride = config & 0xFFFF;
    srcBlockStride = (config >> srcBlockStrideShift) & 0xFFFF;
    srcRepeatStride = (config >> srcRepeatStrideShift) & 0xFFFF;
}

// patternMode mask
//  00         src1
//  01         101010…10
//  02         010101…01
//  03         10001000…1000
//  04         01000100…0100
//  05         00100010…0010
//  06         00010001…0001
//  07         11111111…1111
template<typename TO, typename TI0, typename TI1>
AICORE_FUNC_HEAD void RecordVreduceFunc(EXTRA_PARAMS_DEC, uint8_t patternMode,
    __ubuf__ TO *dst, __ubuf__ TI0 *src0, __ubuf__ TI1 *src1, uint8_t repeat, uint8_t src0BlockStride,
    uint8_t src0RepeatStride, uint8_t src1RepeatStride, uint8_t src1BlockSize)
{
    if (patternMode == 0) {
        RecordBinaryOpFunc<RecordType::VREDUCEV2_BINARY>(EXTRA_PARAMS, dst, src0, src1, repeat, 1, src0BlockStride,
        1, 8, src0RepeatStride, src1RepeatStride, 8, 8, 1, 32, 32, src1BlockSize, true);
    } else if (patternMode < 3U) {
        RecordUnaryOpFunc<RecordType::VREDUCEV2_UNARY>(EXTRA_PARAMS, dst, src0, repeat, 1, src0BlockStride, 4,
            src0RepeatStride, 4, 8, 32, 32, true);
    } else if (patternMode < 7U) {
        RecordUnaryOpFunc<RecordType::VREDUCEV2_UNARY>(EXTRA_PARAMS, dst, src0, repeat, 1, src0BlockStride, 2,
            src0RepeatStride, 2, 8, 32, 32, true);
    } else {
        RecordUnaryOpFunc<RecordType::VREDUCEV2_UNARY>(EXTRA_PARAMS, dst, src0, repeat, 1, src0BlockStride, 8,
            src0RepeatStride, 8, 8, 32, 32, true);
    }
}

template<typename T>
AICORE_FUNC_HEAD void RecordVreducev2Func(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1,
                                           uint16_t repeat, uint8_t src0BlockStride, uint8_t patternMode,
                                           uint16_t src0RepeatStride, uint8_t src1RepeatStride)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    ReduceV2Record record {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src0 = reinterpret_cast<uint64_t>(src0);
    record.src1 = reinterpret_cast<uint64_t>(src1);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.repeat = repeat;
    record.src0BlockStride = src0BlockStride;
    record.patternMode = patternMode;
    record.src0RepeatStride = src0RepeatStride;
    record.src1RepeatStride = src1RepeatStride;
    record.dataBytes = sizeof(T);

    Recorder recorder(memInfo, blockIdx);
    ParseVectorMask(record, recorder, true);
    ParseCompareMask(record.compareMask, src1);
    recorder.DumpRecord<RecordType::VREDUCEV2>(record);
}

/// vmrgsort4指令的Xd单位是字节，910B按32B对齐，310P按数据类型对齐。
///               Xn中储存了4个地址，分别是Xn[15:0], Xn[31:16], Xn[47:32], Xn[63:48]。910B上需要*8B才是真实的UB地址；
///               310P上f16的单位是16B，f32是32B。
/// 指令处理的block大小与Region Proposal的结构定义有关。根据ISA介绍，910B上VBS32/VMS4的排序只考虑score，再加一个index
///               用来包含其他信息，所以VMS4的输入RP大小是8B。而310P区分f16和f32，f16的每个Region Proposal占内存16B，
///               f32的每个Region Proposal占内存32B。
/// repeat > 1时，该指令4个list长度必须保证相等，故写入总长度为：proposalLiNum0 * 8 * 4。必须满足：
///               1.四个lists在UB上连续，2.lists必须长度相等，3.isAllStored必须是0，4.maskSignal是全1（4位bits，取15），
///               若不能同时满足，算子运行会挂。因此repeat>1时，默认4个条件都满足。
/// repeat = 1时，该指令4个list长度不必须相等，maskSignal会指定有效的list，写入总长度为有效list长度和 * 8。
///               ----------------------------------isAllStored----------------------------------
///               如果isAllStored使能，当出现首个list耗尽时（即处理完所有region proposals），指令停止，
///               并将当前每个list上已经处理的region proposal个数存储到寄存器VMS4_SR中。注意，并不是
///               取最短的list长度即可，运行结果是随机的。举例，实测给了4个lists：[16,16,16,8]，当
///               isAllStored使能时，VMS4_SR的值可能是[14,0,16,0]，也可能是[16,0,10,0]，与运行状态有关。
///               由于时序关系是先vmrgsort4再写入寄存器，然后通过get_vms4_sr拿到寄存器的值，因此在解析
///               vmrgsort4时还不知道寄存器中的值，所以这种情况暂时不考虑。
/// repeat = 0时，910B上指令被视为NOP，310P没提到，也按NOP处理，直接返回。
template<RecordType recordType, typename T>
AICORE_FUNC_HEAD void RecordVmrgsort4OpFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src0, uint8_t repeat,
    uint16_t regionProposalLi0, uint16_t regionProposalLi1, uint16_t regionProposalLi2, uint16_t regionProposalLi3,
    bool isAllStored, uint8_t maskSignal)
{
    if (InvalidMemInfo(memInfo) || repeat == 0) {
        return;
    }

    uint64_t xn = reinterpret_cast<uint64_t>(src0);
    uint16_t srcUnit = recordType == RecordType::VMRGSORT4_OP_C220 ? 8U : DataBitSize<T>::value;

    if (repeat == 1) {
        // --------------------------------读写行为抽象--------------------------------
        // 读n次（n取决于maskSignal使能了几个lists），写1次（将所有有效的lists连续写入dst）。
        // 读的次数不定，因此使用结构ElementRecord分别记录“读”和“写”：
        //   [910B]   | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // each read: |   8B    |     1     |  rpLi  |     1     |  1  | bits |    8B     |
        // one write: |   8B    |     1     |totalNum|     1     |  1  | bits |    32B    |
        // --------------------------------------------------------------------------------
        //   [310P]   | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // each read: | dataBit |     1     |  rpLi  |     1     |  1  | bits |  dataBit  |
        // one write: | dataBit |     1     |totalNum|     1     |  1  | bits |  dataBit  |
        // 【注】310P这里blockSize/alignSize单位就是字节，只是大小恰好等于dataBit，比如f16是16B，f32是32B。
        uint64_t blockIdx = GetBlockIdx();
        Recorder recorder(memInfo, blockIdx);
        auto record = ElementRecord();
#if !defined(BUILD_DYNAMIC_PROBE)
        record.location.fileNo = fileNo;
        record.location.lineNo = lineNo;
#endif
        record.location.pc = static_cast<uint64_t>(pc);
        record.location.blockId = blockIdx;
        record.blockSize = srcUnit;
        record.dataBits = DataBitSize<T>::value;
        // n次“读”
        record.accessType = AccessType::READ;
        record.alignSize = record.blockSize;
        uint16_t lists[4] = {regionProposalLi0, regionProposalLi1, regionProposalLi2, regionProposalLi3};
        uint32_t totalNum{};
        for (size_t i = 0; i < 4; ++i) {
            if (((maskSignal >> i) & 0x01) == 1U) {
                record.addr = ((xn >> (16 * i)) & 0xFFFF) * srcUnit;
                record.blockNum = lists[i];
                recorder.DumpRecord<RecordType::ELEMENT>(record);
                totalNum += lists[i];
            }
        }
        // 1次“写”
        record.accessType = AccessType::WRITE;
        record.alignSize = recordType == RecordType::VMRGSORT4_OP_C220 ? 32U : DataBitSize<T>::value;
        record.addr = reinterpret_cast<uint64_t>(dst);
        record.blockNum = totalNum;
        recorder.DumpRecord<RecordType::ELEMENT>(record);
    } else {
        // --------------------------------读写行为抽象--------------------------------
        // 读1次（4个lists连续且长度相等），写1次（将4个lists连续写入dst）。
        // 使用结构UnaryOpRecord记录“读”和“写”：
        // [910B] |    blkSize    | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        //  read: |   rpLi0*4*8   |     1     |    1   |     1     | rpt | bits |     8B    |
        // write: |   rpLi0*4*8   |     1     |    1   |     1     | rpt | bits |    32B    |
        // ----------------------------------------------------------------------------------
        // [310P] |    blkSize    | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        //  read: |rpLi0*4*dataBit|     1     |    1   |     1     | rpt | bits |  dataBit  |
        // write: |rpLi0*4*dataBit|     1     |    1   |     1     | rpt | bits |  dataBit  |
        uint64_t addrList0 = ((xn >> 0) & 0xFFFF) * srcUnit;
        uint64_t blockSize = regionProposalLi0 * 4 * srcUnit;
        RecordUnaryOpFunc<recordType>(EXTRA_PARAMS, dst, reinterpret_cast<__ubuf__ T *>(addrList0),
                                      repeat, 1, 1, 1, 1, 1, 1, blockSize, blockSize, false);
    }
}

template<typename T>
AICORE_FUNC_HEAD void RecordVmrgsort4C220Config(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src0,
                                                 uint64_t src1, uint64_t config)
{
    uint8_t repeat = (config >> 0) & 0xFF;
    uint16_t regionProposalLi0 = (src1 >> 0) & 0xFFFF;
    uint16_t regionProposalLi1 = (src1 >> 16) & 0xFFFF;
    uint16_t regionProposalLi2 = (src1 >> 32) & 0xFFFF;
    uint16_t regionProposalLi3 = (src1 >> 48) & 0xFFFF;
    bool isAllStored = (config >> 12) & 0x1;
    uint8_t maskSignal = (config >> 8) & 0xF;
    RecordVmrgsort4OpFunc<RecordType::VMRGSORT4_OP_C220>(EXTRA_PARAMS, dst, src0, repeat, regionProposalLi0,
        regionProposalLi1, regionProposalLi2, regionProposalLi3, isAllStored, maskSignal);
}

template<typename T>
AICORE_FUNC_HEAD void RecordVmrgsort4M200Config(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src, uint64_t config)
{
    uint8_t repeat = (config >> 0) & 0xFF;
    uint16_t regionProposalLi0 = (config >> 8) & 0xFFF;
    uint16_t regionProposalLi1 = (config >> 20) & 0xFFF;
    uint16_t regionProposalLi2 = (config >> 32) & 0xFFF;
    uint16_t regionProposalLi3 = (config >> 44) & 0xFFF;
    bool isAllStored = (config >> 59) & 0x1;
    uint8_t maskSignal = (config >> 60) & 0xF;
    RecordVmrgsort4OpFunc<RecordType::VMRGSORT4_OP_M200>(EXTRA_PARAMS, dst, src, repeat, regionProposalLi0,
        regionProposalLi1, regionProposalLi2, regionProposalLi3, isAllStored, maskSignal);
}

// 从指令行为上来说，c310和c220一致，但是为了避免不同芯片在未来可能的行为差异，写成不同函数
template<DetailedDataType detailedDataType>
AICORE_FUNC_HEAD void RecordVmrgsort4C310(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src, uint64_t xm, uint64_t xt)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    Vms4v2RecordA5 record{};
    record.detailedDataType = detailedDataType;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.dst = dst;
    for (uint32_t i = 0; i < Vms4v2RecordA5::ARRAY_NUM; ++i) {
        record.src[i] = ((src >> (16 * i)) & 0xFFFF) << Vms4v2RecordA5::SRC_UNIT;
        record.elementNum[i] = (xm >> (16 * i)) & 0xFFFF;
    }
    record.repeat = GetUintFromConf<7, 0>(xt);
    record.validMask = GetUintFromConf<11, 8>(xt);
    record.isAllStored = GetUintFromConf<12, 12>(xt);
    recorder.DumpRecord<RecordType::VMRGSORT4_OP_C310>(record);
}

template<RecordType recordType, typename TO, typename TI>
AICORE_FUNC_HEAD void RecordUnaryOpConfigFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI *src, uint64_t config, uint8_t dstBlockNum, uint8_t srcBlockNum,
    uint16_t dstBlockSize, uint16_t srcBlockSize, bool useMask = true)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseUnaryConfigByArch(config, repeat, dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride);
    RecordUnaryOpFunc<recordType>(EXTRA_PARAMS, dst, src, repeat, dstBlockStride, srcBlockStride,
        dstRepeatStride, srcRepeatStride, dstBlockNum, srcBlockNum, dstBlockSize, srcBlockSize, useMask);
}

template<RecordType recordType, typename TO, typename TI1, typename TI2>
AICORE_FUNC_HEAD void RecordBinaryOpConfigFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI1 *src0, __ubuf__ TI2 *src1, uint64_t config, uint8_t dstBlockNum,
    uint8_t src0BlockNum, uint8_t src1BlockNum, uint16_t dstBlockSize, uint16_t src0BlockSize,
    uint16_t src1BlockSize, bool useMask = true)
{
    auto repeat = uint8_t{};
    auto dstBlockStride = uint8_t{};
    auto src0BlockStride = uint8_t{};
    auto src1BlockStride = uint8_t{};
    auto dstRepeatStride = uint8_t{};
    auto src0RepeatStride = uint8_t{};
    auto src1RepeatStride = uint8_t{};
    ParseBinaryConfig(config, repeat, dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride,
        src0RepeatStride, src1RepeatStride);
    RecordBinaryOpFunc<recordType>(EXTRA_PARAMS, dst, src0, src1, repeat,
        dstBlockStride, src0BlockStride, src1BlockStride, dstRepeatStride, src0RepeatStride, src1RepeatStride,
        dstBlockNum, src0BlockNum, src1BlockNum, dstBlockSize, src0BlockSize, src1BlockSize, useMask);
}

AICORE_FUNC_HEAD void ParseUnaryOpWithOffsetConf(uint64_t config, uint8_t &repeat, uint16_t &dstRepeatStride,
                                                  uint16_t &srcRepeatStride, uint32_t &offsetAddr)
{
    constexpr uint8_t offsetAddrWidth = 32;
    constexpr uint8_t offsetAddrShift = 0;
    constexpr uint8_t dstRepeatStrideShift = 32;
    constexpr uint8_t dstRepeatStrideWidth = 8;
    constexpr uint8_t srcRepeatStrideShift = 40;
    constexpr uint8_t srcRepeatStrideWidth = 8;
    constexpr uint8_t repeatShift = 56;
    constexpr uint8_t repeatWidth = 8;

    offsetAddr = GetIntFromConf<offsetAddrShift, offsetAddrWidth>(config);
    dstRepeatStride = GetIntFromConf<dstRepeatStrideShift, dstRepeatStrideWidth>(config);
    srcRepeatStride = GetIntFromConf<srcRepeatStrideShift, srcRepeatStrideWidth>(config);
    repeat = GetIntFromConf<repeatShift, repeatWidth>(config);
}

AICORE_FUNC_HEAD bool IsParseElementSuccess(uint64_t secondAddr, uint64_t firstAddr, const AccessType& accessType,
                                             Recorder& recorder, ElementRecord& element)
{
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
    uint64_t ubSize = 0x30000ULL;
#else
    uint64_t ubSize = 0x40000ULL;
#endif
    if (secondAddr < ubSize) {
        element.accessType = accessType;
        element.addr = firstAddr;
        recorder.DumpRecord<RecordType::ELEMENT>(element);
        return true;
    }
    return false;
}

AICORE_FUNC_HEAD void ParseElementRecord(const VgatherRecord& record, Recorder& recorder,
                                          __ubuf__ uint32_t* src, uint16_t elementNum, uint8_t elementBlockSize)
{
    auto element = ElementRecord();
    element.location = record.location;
    element.location.blockId = record.location.blockId;
    // elementNum次“读”
    element.blockSize = elementBlockSize;
    element.blockNum = 1U;
    element.alignSize = elementBlockSize;
    element.dataBits = record.dstDataBits;
    uint64_t secondAddr, firstAddr;
    for (uint16_t num = 0; num < elementNum; ++num) {
        secondAddr = record.src + num * 4;
        firstAddr = src[num] + static_cast<uint64_t>(record.offsetAddr);
        if (!IsParseElementSuccess(secondAddr, firstAddr, AccessType::READ, recorder, element)) {
            break;
        }
    }
}

// 判断num对应的mask位是否被掩掉，为1表示有效位，返回false
AICORE_FUNC_HEAD bool IsMaskedData(uint16_t num, const VectorMask& vectorMask)
{
    constexpr uint16_t maskBits = 64U;
    // 前64个数用mask0判断掩码
    if (num < maskBits) {
        return (vectorMask.mask0 & (1ULL << num)) == 0;
    // 后64个数用mask1判断掩码
    } else if (num - maskBits < maskBits) {
        return (vectorMask.mask1 & (1ULL << (num - maskBits))) == 0;
    }
    // 超出128个数，默认被掩掉
    return true;
}

AICORE_FUNC_HEAD void ParseElementRecordMaskNorm(const VgatherRecord& record, Recorder& recorder,
                                                  __ubuf__ uint32_t* src, uint16_t nNum)
{
    auto element = ElementRecord();
    element.location = record.location;
    element.location.blockId = record.location.blockId;
    uint64_t secondAddr, firstAddr;
    // 一次完整“读”
    element.accessType = AccessType::READ;
    element.blockSize = 32U;
    element.blockNum = nNum / BITS_EACH_BYTE * record.dstRepeat;
    element.alignSize = 32U;
    element.dataBits = 32U;
    element.addr = record.src;
    recorder.DumpRecord<RecordType::ELEMENT>(element);
    // N*repeat次掩码后的“读”+“写”
    element.blockSize = record.dstDataBits / BITS_EACH_BYTE;
    element.blockNum = 1U;
    element.alignSize = element.blockSize;
    element.dataBits = record.dstDataBits;
    for (uint8_t rep = 0; rep < record.dstRepeat; ++rep) {
        for (uint16_t num = 0; num < nNum; ++num) {
            if (IsMaskedData(num, record.vectorMask)) {
                continue;
            }
            // 读行为
            secondAddr = record.src + num * 4 + rep * nNum * 4;
            firstAddr = src[num + rep * nNum] + static_cast<uint64_t>(record.offsetAddr);
            if (!IsParseElementSuccess(secondAddr, firstAddr, AccessType::READ, recorder, element)) {
                break;
            }
            // 写行为
            firstAddr = record.dst + num * 4 + rep * record.dstRepeatStride * 32;
            if (!IsParseElementSuccess(firstAddr, firstAddr, AccessType::WRITE, recorder, element)) {
                break;
            }
        }
    }
}

AICORE_FUNC_HEAD void ClassifyVgatherByMaskMode(VgatherRecord& record, Recorder& recorder, uint8_t sizeN,
                                                 uint16_t& elementNum, bool& isMaskNorm)
{
    if (record.maskMode == MaskMode::MASK_NORM && record.vectorMask.mask0 == ~0ULL && record.vectorMask.mask1 == ~0ULL) {
        // 没有mask：VgatherRecord是一组长度完整的读写
        // -------------------------------- vgather ---------------------------------
        //      | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // src: |   32    |     1     | N/8*rpt|     1     |  1  |  32  |    32     |
        // dst: |   32    |     1     |    8   | dstRptSrd | rpt | bits |    32     |
        // -------------------------------- vgatherb --------------------------------
        //      | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // src: |   32    |     1     | N/8*rpt|     1     |  1  |  32  |    32     |
        // dst: |   32    | dstBlkSrd |    8   | dstRptSrd | rpt | bits |    32     |
        record.dstBlockSize = 32U;
        record.srcBlockSize = 32U;
        record.dstBlockNum = 8U;
        record.srcBlockNum = static_cast<uint16_t>(sizeN / 8 * record.dstRepeat);
        recorder.DumpRecord<RecordType::VGATHER>(record);
        // 读N*repeat个ElementRecord：
        // ----------------------------------- vgather ------------------------------------
        //            | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // each read: |  bits/8 |     1     |    1   |     1     |  1  | bits |  bits/8   |
        // ----------------------------------- vgatherb -----------------------------------
        //            | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // each read: |   32    |     1     |    1   |     1     |  1  | bits |     32    |
        // -------------------------------------- 注 --------------------------------------
        // bits用来计算AlignSize，明确给出AlignSize的情况下可忽略
        elementNum = record.srcBlockNum * 8;
    } else if (record.maskMode == MaskMode::MASK_COUNT) {
        // mask count：VgatherRecord是一组只有前count个元素的读写，当count个数大于N*rpt，以count为准
        //      | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // src: |    4    |     1     |  count |     1     |  1  |  32  |    32     |
        // dst: |  bits/8 |     1     |  count |     1     |  1  | bits |    32     |
        record.dstBlockSize = static_cast<uint32_t>(record.dstDataBits / BITS_EACH_BYTE);
        record.srcBlockSize = 4U;
        record.dstRepeatStride = 1U;
        record.dstRepeat = 1U;
        record.dstBlockNum = static_cast<uint16_t>(record.vectorMask.mask0);
        record.srcBlockNum = static_cast<uint16_t>(record.vectorMask.mask0);
        recorder.DumpRecord<RecordType::VGATHER>(record);
        // 读count个ElementRecord：
        //            | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // each read: |  bits/8 |     1     |    1   |     1     |  1  | bits |  bits/8   |
        elementNum = record.srcBlockNum;
    } else {
        // (1*完整长度读行为 + 2*掩码后的N*repeat)个ElementRecord
        //            | blkSize | blkStride | blkNum | rptStride | rpt | bits | alignSize |
        // full read: |   32    |     1     | N/8*rpt|     1     |  1  |  32  |    32     |
        // each read: |  bits/8 |     1     |    1   |     1     |  1  | bits |  bits/8   |
        // each write:|  bits/8 |     1     |    1   |     1     |  1  | bits |  bits/8   |
        isMaskNorm = true;
        elementNum = sizeN;
    }
}

// 一个Vgather(b)分两类读（先读二级指针，再读一级指针）和一类写
// 如果没有mask：拆成1次读写（完整长度） + N*repeat次读，即一个VgatherRecord + N*repeat个ElementRecord
// 如果是mask norm：拆成1次读（完整长度） + 掩码后的N*repeat次读写，即1 + 2*掩码后的N*repeat个ElementRecord
// 如果是mask count：拆成1次读写（前count个元素）+ count次读，即一个VgatherRecord + count个ElementRecord
template<typename T>
AICORE_FUNC_HEAD void RecordVgatherOpFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ uint32_t *src,
                                           uint32_t offsetAddr, uint16_t dstRepeatStride, uint8_t dstBlockStride,
                                           uint8_t repeat, uint8_t sizeN, bool useMask)
{
    if (InvalidMemInfo(memInfo)) { return;}
    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    auto record = VgatherRecord();
    ParseVectorMask(record, recorder, useMask);
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src = reinterpret_cast<uint64_t>(src);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.offsetAddr = offsetAddr;
    record.location.blockId = blockIdx;
    record.dstRepeatStride = dstRepeatStride;
#if defined(__DAV_C220__) || defined(__DAV_C220_VEC__) || defined(__DAV_C220_CUBE__)
    record.dstAlignSize = 32U;
#else
    record.dstAlignSize = static_cast<uint16_t>(record.dstDataBits / BITS_EACH_BYTE);
#endif
    record.dstBlockStride = dstBlockStride;
    record.dstRepeat = repeat;
    record.dstDataBits = DataBitSize<T>::value;

    uint16_t elementNum{};
    bool isMaskNorm{};
    ClassifyVgatherByMaskMode(record, recorder, sizeN, elementNum, isMaskNorm);

    // 如果src为空指针，不再解析elements
    if (src == nullptr) { return;}
    if (!isMaskNorm) {
        uint8_t elementBlockSize = useMask ? (record.dstDataBits / BITS_EACH_BYTE) : 32U;
        ParseElementRecord(record, recorder, src, elementNum, elementBlockSize);
    } else {
        ParseElementRecordMaskNorm(record, recorder, src, elementNum);
    }
}

template<typename T>
AICORE_FUNC_HEAD void RecordVgatherOpConfFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ uint32_t *src,
                                               uint64_t config)
{
    uint32_t offsetAddr = (config & 0xffffffff);
    uint16_t dstRepeatStride = (((config >> 44) & 0xf00) | ((config >> 32) & 0xff));
    uint8_t repeat = ((config >> 56) & 0xff);
    uint8_t sizeN = static_cast<uint8_t>(std::is_same<T, uint32_t>::value ? 64 : 128);
    RecordVgatherOpFunc(EXTRA_PARAMS, dst, src, offsetAddr, dstRepeatStride, 1U, repeat, sizeN, true);
}

template<typename T>
AICORE_FUNC_HEAD void RecordVgatherbConfigFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ uint32_t *src,
                                                uint64_t config)
{
    uint32_t offsetAddr = (config & 0xffffffff);
    uint16_t dstRepeatStride = (((config >> 44) & 0xf00) | ((config >> 32) & 0xff));
    uint8_t dstBlockStride = ((config >> 40) & 0xff);
    uint8_t repeat = ((config >> 56) & 0xff);
    RecordVgatherOpFunc(EXTRA_PARAMS, dst, src, offsetAddr, dstRepeatStride, dstBlockStride, repeat, 8, false);
}

template<typename dstT, typename srcT>
AICORE_FUNC_HEAD void RecordVscatterOpFunc(EXTRA_PARAMS_DEC, __ubuf__ dstT *dst, __ubuf__ srcT *src,
                                     uint32_t offset, bool strideSizeMode, bool repeatUpdateMode,
                                     uint8_t repeatTimes, uint8_t srcRepeatStrideSize)
{
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeatTimes, 1, 1, 1, srcRepeatStrideSize,
                                            8, 8, 32, 32);
}

template<typename dstT, typename srcT>
AICORE_FUNC_HEAD void RecordVscatterOpConfFunc(EXTRA_PARAMS_DEC, __ubuf__ dstT *dst, __ubuf__ srcT *src,
       uint64_t config)
{
    constexpr uint8_t strideSizeModeShift = 55;
    constexpr uint8_t repeatUpdateModeShift = 54;
    uint32_t offset{};
    bool strideSizeMode{};
    bool repeatUpdateMode{};
    uint8_t repeatTimes{};
    uint16_t dstRepeatStrideSize{};
    uint16_t srcRepeatStrideSize{};
    ParseUnaryOpWithOffsetConf(config, repeatTimes,  dstRepeatStrideSize, srcRepeatStrideSize, offset);
    strideSizeMode = ((config >> strideSizeModeShift) & 0x1);
    repeatUpdateMode = ((config >> repeatUpdateModeShift) & 0x1);
    RecordVscatterOpFunc(EXTRA_PARAMS, dst, src, offset, strideSizeMode, repeatUpdateMode, repeatTimes,
                         srcRepeatStrideSize);
}

template <RecordType recordType>
AICORE_FUNC_HEAD void RecordMatrixMulOpFunc(EXTRA_PARAMS_DEC,
    __ubuf__ void *dst, __ubuf__ void *src0, __ubuf__ void *src1, uint16_t m, uint16_t k, uint16_t n,
    uint16_t src0repeat, uint16_t src0RepeatStride, uint16_t dstBlockNum, uint16_t src0BlockNum, uint16_t src1BlockNum,
    uint16_t dstBlockSize, uint16_t src0BlockSize, uint16_t src1BlockSize, bool cmatrixSource, bool cmatrixInitVal,
    uint16_t dstAlignSize, uint16_t src0AlignSize, uint16_t src1AlignSize, bool enUnitFlag)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    auto record = MarixMulOpRecord {};
    record.dst = reinterpret_cast<uint64_t>(dst);
    record.src0 = reinterpret_cast<uint64_t>(src0);
    record.src1 = reinterpret_cast<uint64_t>(src1);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    record.m = m;
    record.k = k;
    record.n = n;
    record.src0Repeat = src0repeat;
    record.src0RepeatStride = src0RepeatStride;
    record.dstBlockNum = dstBlockNum;
    record.src0BlockNum = src0BlockNum;
    record.src1BlockNum = src1BlockNum;
    record.dstBlockSize = dstBlockSize;
    record.src0BlockSize = src0BlockSize;
    record.src1BlockSize = src1BlockSize;
    record.cmatrixSource = cmatrixSource;
    record.cmatrixInitVal = cmatrixInitVal;
    record.dstAlignSize = dstAlignSize;
    record.src0AlignSize = src0AlignSize;
    record.src1AlignSize = src1AlignSize;
    record.enUnitFlag = enUnitFlag;
    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<recordType>(record);
}

template<typename T>
AICORE_FUNC_HEAD void RecordViouOpFunc(EXTRA_PARAMS_DEC, __ubuf__ T *dst, __ubuf__ T *src0, __ubuf__ T *src1,
                                        uint64_t config)
{
    constexpr uint8_t RepeatShift = 56;
    constexpr uint8_t ElementSize = sizeof(T);
    constexpr uint8_t ElementNum = 8;
    constexpr uint8_t PropSize = ElementSize * ElementNum;
    constexpr uint16_t PropNum = 16;
    constexpr uint16_t ResultSize = PropNum * PropNum * ElementSize;
    uint8_t repeat = config >> RepeatShift & 0xFF;
    RecordBinaryOpFunc<RecordType::BINARY_OP>(EXTRA_PARAMS, dst, src0, src1, repeat, 1, 1, 0, 1, 1, 1, 1, 1, 1,
                                              ResultSize, PropSize * PropNum, PropSize * PropNum);
}

AICORE_FUNC_HEAD void ParseMadConfig(uint64_t config, uint16_t &m, uint16_t &k,
    uint16_t &n, uint8_t &unitFlag, bool &kDirectionAlign, bool &cMatrixSource, bool&cMatrixValueControl)
{
    constexpr uint64_t kShift = 12;
    constexpr uint64_t nShift = 24;
    constexpr uint64_t unitFlagShift = 55;
    constexpr uint64_t kDirectionAlignShift = 58;
    constexpr uint64_t cMatrixSourceShift = 62;
    constexpr uint64_t cMatrixValueControlShift = 63;
    m = config & 0xFFF;
    k = (config >> kShift) & 0xFFF;
    n = (config >> nShift) & 0xFFF;
    unitFlag = (config >> unitFlagShift) & 0x3;
    kDirectionAlign = (config >> kDirectionAlignShift) & 0x1;
    cMatrixSource = (config >> cMatrixSourceShift) & 0x1;
    cMatrixValueControl = (config >> cMatrixValueControlShift) & 0x1;
}

AICORE_FUNC_HEAD uint16_t CeilToFractal(uint16_t realSize, uint16_t fractalSize)
{
    if (fractalSize == 0) {
        return fractalSize;
    }
    return (realSize + fractalSize - 1) / fractalSize;
}

constexpr uint16_t FRACTAL_SIZE16 = 16;
constexpr uint16_t FRACTAL_SIZE4 = 4;
AICORE_FUNC_HEAD void RecordMatrixOpFunc(EXTRA_PARAMS_DEC, __cc__ void *dst,
    __ca__ void *src0, __cb__ void *src1, uint16_t m, uint16_t k, uint16_t n, uint8_t unitFlag, bool kDirectionAlign,
    bool cmatrixSource, bool cmatrixInitVal, uint16_t dataBitsDst, uint16_t dataBitsSrc0, uint16_t dataBitsSrc1,
    uint16_t dstAlignSize = 1024, uint16_t src0AlignSize = 512, uint16_t src1AlignSize = 512)
{
    if (m == 0 || k == 0 || n == 0 || dataBitsSrc0 == 0) {
        return;
    }
    uint16_t k0 = 256 / dataBitsSrc0; // matrixA分形16xk0
    uint16_t kA = CeilToFractal(k, k0);
    uint16_t mA = CeilToFractal(m, FRACTAL_SIZE16);
    uint16_t src0RepeatStride = kA;
    // f322f32下,kDirectionAlign为1时LOA/LOB矩阵在K方向16对齐，否则为8
    if (kDirectionAlign && (dataBitsSrc0 == 32) && (dataBitsSrc1 == 32)) {
        src0RepeatStride = CeilToFractal(k, FRACTAL_SIZE16) * 2;
    }
    auto k1 = uint16_t{}; // matrixB分形k1xn0

    // u8|s8|u8s8当N<=4, matrixB分形为k1x4
    uint16_t n0 = ((n <= 4) && (dataBitsSrc0 == 8) && (dataBitsSrc1 == 8)) ? FRACTAL_SIZE4 : FRACTAL_SIZE16;
    if (dataBitsSrc1 == 2) {
        k1 = 256 / dataBitsSrc0; // f16u2与f162f16、b8u2与u8分形相同
    } else {
        k1 = 256 / dataBitsSrc1;
    }
    uint16_t kB = CeilToFractal(k, k1);
    uint16_t nB = CeilToFractal(n, n0);

    uint64_t addrSrc0 = reinterpret_cast<uint64_t>(src0);
    uint64_t addrSrc1 = reinterpret_cast<uint64_t>(src1);
    auto dstAddr = uint64_t{};
    // cmatrixSource仅影响Matrix C inint value的来源。dst地址对应xd[0:31]
    dstAddr = reinterpret_cast<uint64_t>(dst) & 0xFFFFFFFF;
    bool enUnitFlag = (unitFlag > 1);  // Mode2/Mode3表示使能unit-flag机制

    RecordMatrixMulOpFunc<RecordType::MATRIX_MUL_OP>(EXTRA_PARAMS, reinterpret_cast<__ubuf__ void *>(dstAddr),
        reinterpret_cast<__ubuf__ void *>(addrSrc0), reinterpret_cast<__ubuf__ void *>(addrSrc1), m, k, n, mA,
        src0RepeatStride, mA * nB, kA, kB * nB, FRACTAL_SIZE16 * FRACTAL_SIZE16 * dataBitsDst / 8,
        k0 * FRACTAL_SIZE16 * dataBitsSrc0 / 8, k1 * n0 * dataBitsSrc1 / 8, cmatrixSource, cmatrixInitVal,
        dstAlignSize, src0AlignSize, src1AlignSize, enUnitFlag);
}

AICORE_FUNC_HEAD void RecordMatrixOpConfigFunc(EXTRA_PARAMS_DEC,
    __cc__ void *dst, __ca__ void *src0, __cb__ void *src1, uint64_t config, uint16_t dataBitsDst,
    uint16_t dataBitsSrc0, uint16_t dataBitsSrc1, uint16_t dstAlignSize = 1024,
     uint16_t src0AlignSize = 512, uint16_t src1AlignSize = 512)
{
    auto m = uint16_t{};
    auto k = uint16_t{};
    auto n = uint16_t{};
    auto unitFlag = uint8_t{};
    auto kDirectionAlign = bool{};
    auto cMatrixSource = bool{}; // matrixC源地址控制位
    auto cMatrixValueControl = bool{}; // matrixC累加控制位
    ParseMadConfig(config, m, k, n, unitFlag, kDirectionAlign, cMatrixSource, cMatrixValueControl);
    RecordMatrixOpFunc(EXTRA_PARAMS, dst, src0, src1, m, k, n, unitFlag, kDirectionAlign, cMatrixSource,
        cMatrixValueControl, dataBitsDst, dataBitsSrc0, dataBitsSrc1, dstAlignSize, src0AlignSize, src1AlignSize);
}

template<DetailedDataType src0Dtype, DetailedDataType src1Dtype>
AICORE_FUNC_HEAD void RecordMmadA5(EXTRA_PARAMS_DEC, __cc__ void *dst, __ca__ void *src0, __cb__ void *src1,
                                    uint64_t config, uint16_t srcAlign = 512)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }
    uint64_t blockIdx = GetBlockIdx();
    MmadA5Record record{};
    record.src0Dtype = src0Dtype;
    record.src1Dtype = src1Dtype;
    record.location.blockId = blockIdx;
    record.dst = GetUintFromConf<31, 0>(reinterpret_cast<uint64_t>(dst));
    record.src0 = reinterpret_cast<uint64_t>(src0);
    record.src1 = reinterpret_cast<uint64_t>(src1);
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.m = GetUintFromConf<11, 0>(config);
    record.k = GetUintFromConf<23, 12>(config);
    record.n = GetUintFromConf<35, 24>(config);
    record.enUnitFlag = GetUintFromConf<56, 55>(config) > 1; // 2&3 means enable unit-flag

    bool enGEMV = GetUintFromConf<61, 61>(config) == 0;
    uint16_t k0A = 256 / GetDataBits(src0Dtype);
    uint16_t k0B = 256 / GetDataBits(src1Dtype);
    uint32_t m = CeilToFractal(record.m, FRACTAL_SIZE16);
    uint32_t kA = CeilToFractal(record.k, k0A); // k is 32B align
    uint32_t kB = CeilToFractal(record.k, k0B);
    uint32_t n = CeilToFractal(record.n, FRACTAL_SIZE16);
    record.cmatrixSource = GetUintFromConf<62, 62>(config);
    record.cmatrixInitVal = GetUintFromConf<63, 63>(config);
    record.src0BlockNum = m * kA;
    if (enGEMV && record.m == 1) {
        record.src0BlockNum /= 16; // GEMV 模式下 src0 会从 1*k 的矩阵变为 16*(k/16) 的矩阵；dst矩阵会从 1*n 变为 16*n
    }
    record.src1BlockNum = n * kB;
    record.src0AlignSize = record.src1AlignSize = srcAlign;
    record.dstBlockNum = m * n;
    Recorder recorder(memInfo, blockIdx);
    recorder.DumpRecord<RecordType::MMAD_A5>(record);
}

template<RecordType recordType, typename TO, typename TI>
AICORE_FUNC_HEAD void RecordReduceOpConfigFunc(EXTRA_PARAMS_DEC,
    __ubuf__ TO *dst, __ubuf__ TI *src, uint64_t config, uint16_t dstRepeatLength, uint8_t dstBlockNum,
    uint8_t srcBlockNum, uint8_t dstBlockSize, uint8_t srcBlockSize, uint16_t dstAlignSize = 32,
    uint8_t dstDataBitsFactor = 1)
{
    auto repeat = uint8_t{};
    auto dstRepeatStride = uint16_t{};
    auto srcBlockStride = uint16_t{};
    auto srcRepeatStride = uint16_t{};
    ParseReduceConfig(config, repeat, dstRepeatStride, srcBlockStride, srcRepeatStride);
    RecordReduceOpFunc<recordType, TO, TI>(EXTRA_PARAMS, dst, src, repeat, dstRepeatStride, srcBlockStride,
        srcRepeatStride, dstRepeatLength, dstBlockNum, srcBlockNum, dstBlockSize, srcBlockSize, dstAlignSize,
        dstDataBitsFactor);
}

// For VCMAX and VCMIN
template <typename T,
    typename = typename std::enable_if<
    std::is_same<T, half>::value ||
    std::is_same<T, float>::value >::type
>
AICORE_FUNC_HEAD void ParseOrder(OrderType order, uint16_t &dstRepeatLength, uint8_t &dstDataBitsFactor,
                                    uint16_t &dstAlignUnit)
{
    if (std::is_same<T, half>::value) {
        dstAlignUnit = 4U;
    } else {
        dstAlignUnit = 8U;
    }
    dstDataBitsFactor = 1;
    if ((order == OrderType::VALUE_INDEX) || (order == OrderType::INDEX_VALUE)) {
        dstRepeatLength = static_cast<uint16_t>(OrderWriteMode::WRITE_TWICE) * sizeof(T);
        dstDataBitsFactor = 2;
    } else if (order == OrderType::ONLY_VALUE) {
        dstRepeatLength = static_cast<uint16_t>(OrderWriteMode::WRITE_ONCE) * sizeof(T);
        dstAlignUnit >>= 1;
    } else {
        dstRepeatLength = static_cast<uint16_t>(OrderWriteMode::WRITE_ONCE) * 4;
        if (std::is_same<T, float>::value) {
            dstAlignUnit >>= 1;
        }
    }
}

AICORE_FUNC_HEAD void RecordScatterVnchwconvEvent(EXTRA_PARAMS_DEC, ub_addr8_t dst, ub_addr8_t src, uint8_t repeat,
                                                   uint8_t dstStride, uint8_t srcStride, DataType dataType,
                                                   bool dstHighHalf = false, bool srcHighHalf = false)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    auto record = ScatterVnchwconvRecord{};
    ParseVaRegister(recorder, record.dst0, record.dst1, dst);
    ParseVaRegister(recorder, record.src0, record.src1, src);
    record.repeat = repeat;
    record.dstStride = dstStride;
    record.srcStride = srcStride;
    record.dstHighHalf = dstHighHalf;
    record.srcHighHalf = srcHighHalf;
    record.dataType = dataType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    recorder.DumpRecord<RecordType::SCATTERVNCHWCONV>(record);
}
 
AICORE_FUNC_HEAD void RecordScatterVnchwconvEvent(EXTRA_PARAMS_DEC, ub_addr8_t dst, ub_addr8_t src,
                                                   uint64_t config, DataType dataType,
                                                   bool dstHighHalf = false, bool srcHighHalf = false)
{
    const uint8_t dstStride = config & 0xffff;
    const uint8_t srcStride = (config >> 16U) & 0xffff;
    const uint8_t repeat = (config >> 56U) & 0xff;
    RecordScatterVnchwconvEvent(EXTRA_PARAMS, dst, src, repeat, dstStride, srcStride,
                                dataType, dstHighHalf, srcHighHalf);
}

AICORE_FUNC_HEAD void RecordScatterVnchwconvA5(EXTRA_PARAMS_DEC, ub_addr8_t dst, ub_addr8_t src,
                                                uint64_t config, DataType dataType,
                                                bool dstHighHalf = false, bool srcHighHalf = false)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    ScatterVnchwconvRecord record{};
    ParseVaRegister(recorder, record.dst0, record.dst1, dst);
    ParseVaRegister(recorder, record.src0, record.src1, src);
    record.repeat = GetUintFromConf<63, 56>(config);
    record.dstStride = GetUintFromConf<15, 0>(config);
    record.srcStride = GetUintFromConf<31, 16>(config);
    record.dstHighHalf = dstHighHalf;
    record.srcHighHalf = srcHighHalf;
    record.dataType = dataType;
#if !defined(BUILD_DYNAMIC_PROBE)
    record.location.fileNo = fileNo;
    record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.location.blockId = blockIdx;
    recorder.DumpRecord<RecordType::SCATTERVNCHWCONV_A5>(record);
}

template<DataType dataType>
AICORE_FUNC_HEAD void RecordVbs32A5(EXTRA_PARAMS_DEC, uint64_t dst, uint64_t src0, uint64_t src1, uint64_t config)
{
    if (InvalidMemInfo(memInfo)) {
        return;
    }

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    Vbs32Record record{};
    record.dataType = dataType;
    record.location.blockId = blockIdx;
#if !defined(BUILD_DYNAMIC_PROBE)
        record.location.fileNo = fileNo;
        record.location.lineNo = lineNo;
#endif
    record.location.pc = static_cast<uint64_t>(pc);
    record.dst = dst;
    record.src0 = src0;
    record.src1 = src1;
    record.repeat = GetUintFromConf<63, 56>(config);
    recorder.DumpRecord<RecordType::VBS32_A5>(record);
}

}  // namespace Sanitizer

#endif  // PLUGIN_RECORD_CALC_INSTRUCTIONS_H
