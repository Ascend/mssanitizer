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

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510)

SANITIZER_REPORT(set_vector_mask, uint64_t reg_idx, uint64_t reg_value)
{
    (void)reg_idx;
    (void)reg_value;
}

SANITIZER_REPORT(set_ctrl, uint64_t config)
{
    (void)config;
}

SANITIZER_REPORT(set_ffts_base_addr, uint64_t config)
{
    (void)config;
}

SANITIZER_REPORT(set_fpc, uint64_t config)
{
    (void)config;
}

SANITIZER_REPORT(set_quant_pre, uint64_t config)
{
    (void)config;
}

SANITIZER_REPORT(set_quant_post, uint64_t config)
{
    (void)config;
}

SANITIZER_REPORT(set_lrelu_alpha, uint64_t config)
{
    (void)config;
}

#else

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

SANITIZER_REPORT(set_lrelu_alpha, uint64_t config)
{
    RecordLreluAlpha(EXTRA_PARAMS, config, RegisterValueType::VAL_UINT64);
}

#endif