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


#include "plugin/kernel_pub_func.h"
#include "plugin/utils.h"
#include "plugin/record_calc_instructions.h"

using namespace Sanitizer;
SANITIZER_REPORT(VMERGECH_b8, __ubuf__ uint8_t *dst, __ubuf__ uint8_t *src, uint64_t config)
{
    uint8_t repeat = config >> 56;
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, 1, 1, 1, 1, 1, 1, 32, 256);
}

SANITIZER_REPORT(VMERGECH_f16, __ubuf__ half *dst, __ubuf__ half *src, uint64_t config)
{
    uint8_t repeat = config >> 56;
    RecordUnaryOpFunc<RecordType::UNARY_OP>(EXTRA_PARAMS, dst, src, repeat, 1, 1, 1, 1, 1, 1, 64, 256);
}
