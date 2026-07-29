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


#include "plugin/record_calc_instructions.h"

using namespace Sanitizer;
#if !(defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1 && defined(__DAV_C220_CUBE__))
// #2910
SANITIZER_REPORT(vcopy, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

 // #2911
SANITIZER_REPORT(vcopy, __ubuf__ int16_t *dst, __ubuf__ int16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2912
SANITIZER_REPORT(vcopy, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

 // #2913
SANITIZER_REPORT(vcopy, __ubuf__ int32_t *dst, __ubuf__ int32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2914
SANITIZER_REPORT(vcopy, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

 // #2915
SANITIZER_REPORT(vcopy, __ubuf__ uint16_t *dst, __ubuf__ uint16_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

// #2916
SANITIZER_REPORT(vcopy, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint64_t config)
{
    RecordUnaryOpConfigFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, config, 8, 8, 32, 32);
}

 // #2917
SANITIZER_REPORT(vcopy, __ubuf__ uint32_t *dst, __ubuf__ uint32_t *src, uint8_t repeat,
    uint16_t dstBlockStride, uint16_t srcBlockStride, uint16_t dstRepeatStride, uint16_t srcRepeatStride)
{
    RecordUnaryOpFunc<RecordType::VCOPY_OP, InstrName::VCOPY>(EXTRA_PARAMS, dst, src, repeat,
        dstBlockStride, srcBlockStride, dstRepeatStride, srcRepeatStride, 8, 8, 32, 32);
}

#endif
