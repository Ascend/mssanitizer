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


#include "plugin/utils.h"
#include "plugin/record_ctrl_instructions.h"

using namespace Sanitizer;

// #2176
SANITIZER_REPORT(set_mask_count)
{
    RecordRegister(EXTRA_PARAMS, &Register::maskMode, MaskMode::MASK_COUNT);

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    uint64_t ctrlVal = 0;
    recorder.GetRegister(&Register::ctrl, ctrlVal);
    ctrlVal |= (1UL << 56);     // sbitset1
    RecordSetRegister<RecordType::SET_CTRL>(EXTRA_PARAMS, &Register::ctrl, ctrlVal, RegisterValueType::VAL_UINT64);
}

// #2177
SANITIZER_REPORT(set_mask_norm)
{
    RecordRegister(EXTRA_PARAMS, &Register::maskMode, MaskMode::MASK_NORM);

    uint64_t blockIdx = GetBlockIdx();
    Recorder recorder(memInfo, blockIdx);
    uint64_t ctrlVal = 0;
    recorder.GetRegister(&Register::ctrl, ctrlVal);
    ctrlVal &= ~(1UL << 56);    // sbitset0
    RecordSetRegister<RecordType::SET_CTRL>(EXTRA_PARAMS, &Register::ctrl, ctrlVal, RegisterValueType::VAL_UINT64);
}

// #2208
SANITIZER_REPORT(set_vector_mask, uint64_t reg_idx, uint64_t reg_value)
{
    RecordVectorMask(EXTRA_PARAMS, reg_idx, reg_value);
}

SANITIZER_REPORT(set_ctrl, uint64_t config)
{
    RecordSetRegister<RecordType::SET_CTRL>(EXTRA_PARAMS, &Register::ctrl, config, RegisterValueType::VAL_UINT64);
}

SANITIZER_REPORT(set_ffts_base_addr, uint64_t config)
{
    RecordSetRegister<RecordType::SET_FFTS_BASE_ADDR>(EXTRA_PARAMS, &Register::fftsBaseAddr, config,
        RegisterValueType::VAL_UINT64);
}

SANITIZER_REPORT(set_fpc, uint64_t config)
{
    RecordSetRegister<RecordType::SET_FPC>(EXTRA_PARAMS, &Register::fpc, config, RegisterValueType::VAL_UINT64);
}

SANITIZER_REPORT(set_quant_pre, uint64_t config)
{
    RecordSetRegister<RecordType::SET_QUANT_PRE>(EXTRA_PARAMS, &Register::quantPre, config,
        RegisterValueType::VAL_UINT64);
}

SANITIZER_REPORT(set_quant_post, uint64_t config)
{
    RecordSetRegister<RecordType::SET_QUANT_POST>(EXTRA_PARAMS, &Register::quantPost, config,
        RegisterValueType::VAL_UINT64);
}

// 与编译器约定：set_lrelu_alpha指令的静态插桩维持half和float类型区分；动态插桩统一使用uint64参数，通过L0CToL1Event区分具体类型
SANITIZER_REPORT(set_lrelu_alpha, half config)
{
    RecordLreluAlpha(EXTRA_PARAMS, config, RegisterValueType::VAL_HALF);
}

SANITIZER_REPORT(set_lrelu_alpha, float config)
{
    RecordLreluAlpha(EXTRA_PARAMS, config, RegisterValueType::VAL_FLOAT);
}

// #3023
SANITIZER_REPORT(set_nd_para, uint64_t config) {
    RecordNdPara(EXTRA_PARAMS, config);
}

SANITIZER_REPORT(MOVEVA, ub_addr8_t addr, uint32_t bitOffset, uint64_t xn, uint64_t xm) {
    RecordVAdPara(EXTRA_PARAMS, addr, bitOffset, xn, xm);
}