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

#ifndef TEST_PLUGIN_CCEC_CCEC_DEFS_H
#define TEST_PLUGIN_CCEC_CCEC_DEFS_H

#include "core/framework/record_defs.h"

constexpr uint64_t UT_RECORD_BUF_SIZE = 1024 * 1024 * 16;

#ifndef __DAV_C220__
#define __DAV_C220__
#endif // __DAV_C220__

#ifndef __DAV_C220_CUBE__
#define __DAV_C220_CUBE__
#endif // __DAV_C220_CUBE__

#ifndef __DAV_C220_VEC__
#define __DAV_C220_VEC__
#endif // __DAV_C220_VEC__

template<typename DstType, typename Src0Type = DstType, typename Src1Type = Src0Type, typename... Args>
using UnaryScalarConfigFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t,
                                       DstType*, Src0Type*, Src1Type, uint64_t, Args...);

template<typename DstType, typename Src0Type = DstType, typename Src1Type = Src0Type, typename... Args>
using UnaryScalarU16Func = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, Src0Type*, Src1Type,
                                       uint8_t, uint16_t, uint16_t, uint16_t, uint16_t, Args...);

template<typename DstType, typename Src0Type = DstType, typename Src1Type = Src0Type, typename... Args>
using UnaryScalarU8Func = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, Src0Type*, Src1Type,
                                       uint8_t, uint16_t, uint16_t, uint8_t, uint8_t, Args...);

template<typename DstType, typename SrcType = DstType, typename... Args>
using UnaryConfigFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, SrcType*, uint64_t, Args...);

template<typename DstType, typename SrcType = DstType, typename... Args>
using UnaryU16Func = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, SrcType*,
                              uint8_t, uint16_t, uint16_t, uint16_t, uint16_t, Args...);

template<typename DstType, typename SrcType = DstType, typename... Args>
using UnaryU8Func = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, SrcType*,
                             uint8_t, uint16_t, uint16_t, uint8_t, uint8_t, Args...);

template<typename DstType, typename Src0Type = DstType, typename Src1Type = Src0Type, typename... Args>
using BinaryConfigFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t,
                                  DstType*, Src0Type*, Src1Type*, uint64_t, Args...);

template<typename DstType, typename Src0Type = DstType, typename Src1Type = Src0Type, typename... Args>
using BinaryFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, Src0Type*, Src1Type*,
                              uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, Args...);

template<typename DstType, typename SrcType = DstType, typename... Args>
using ReduceConfigFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, SrcType*, uint64_t, Args...);

template<typename DstType, typename SrcType = DstType, typename... Args>
using ReduceFunc = void (*)(uint8_t*, uint64_t, uint64_t, int64_t, DstType*, SrcType*,
                              uint8_t, uint16_t, uint16_t, uint16_t, Args...);

#endif  // TEST_PLUGIN_CCEC_CCEC_DEFS_H
