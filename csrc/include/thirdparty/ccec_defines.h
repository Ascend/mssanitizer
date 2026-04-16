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

#ifndef CCEC_DEFINES_H
#define CCEC_DEFINES_H

// 毕昇编译器中以 [aicore] 标记一个函数编译成 kenrel 侧代码，为了保证代码同时可在
// host 侧进行单元测试，需要针对是否为 ccec 编译器区分处理。
#if defined(__CCE_IS_AICORE__) && __CCE_IS_AICORE__ == 1

#define __aicore__ [aicore]

// 编译器在插桩时拿不到枚举类型，只能拿到对应的整型类型，因此我们实现桩接口时需要
// 把内置的枚举类型替换为整型防止链接错误。后续编译器支持解析枚举后再去掉此映射
#define ConvRelu_t uint32_t
#define pipe_t uint32_t
#define mem_t uint32_t
#define Order_t uint32_t
#define pad_t uint32_t
#define bm_t uint32_t
#define vpart_t uint32_t
#define ub_addr8_t uint32_t
#define unit_flag_t uint32_t
#define addr_cal_mode_t uint32_t
#define event_t uint32_t
#else

#ifndef __NPU_ARCH__
#define __NPU_ARCH__ 3101
#endif  // __NPU_ARCH__

#ifndef __DAV_VEC__
#define __DAV_VEC__
#endif  // __DAV_VEC__

#ifndef __DAV_CUBE__
#define __DAV_CUBE__
#endif  // __DAV_CUBE__

#ifndef __aicore__
#define __aicore__
#endif  // __aicore__

#ifndef __DAV_C220__
#define __DAV_C220__
#endif  // __DAV_C220__

#ifndef __DAV_C220_VEC__
#define __DAV_C220_VEC__
#endif  // __DAV_C220_VEC__

#ifndef __DAV_C220_CUBE__
#define __DAV_C220_CUBE__
#endif  // __DAV_C220_CUBE__

#ifndef __DAV_M200__
#define __DAV_M200__
#endif  // __DAV_M200__

#ifndef __DAV_M200_VEC__
#define __DAV_M200_VEC__
#endif  // __DAV_M200_VEC__

#ifndef __ubuf__
#define __ubuf__
#endif  // __ubuf__

#ifndef __cbuf__
#define __cbuf__
#endif  // __cbuf__

#ifndef __ca__
#define __ca__
#endif  // __ca__

#ifndef __cb__
#define __cb__
#endif  // __cb__

#ifndef __gm__
#define __gm__
#endif  // __gm__

#ifndef __cc__
#define __cc__
#endif // __cc__

#ifndef __ca__
#define __ca__
#endif // __ca__

#ifndef __cb__
#define __cb__
#endif // __cb__

#ifndef __fbuf__
#define __fbuf__
#endif  // __fbuf__

#ifndef pipe_t
#define pipe_t PipeType
#endif  // pipe_t

#ifndef mem_t
#define mem_t MemType
#endif  // mem_t

#ifndef event_t
#define event_t EventID
#endif  // event_t

#ifndef Order_t
#define Order_t OrderType
#endif // Order_t

#ifndef pad_t
#define pad_t PadMode
#endif // pad_t

#ifndef unit_flag_t
#define unit_flag_t UintFlag
#endif // unit_flag_t

#ifndef bm_t
#define bm_t ByteMode
#endif // bm_t

#ifndef half
struct Half {
    char val[2];
};
#define half struct Half
#endif  // half

#ifndef ub_addr8_t
#define ub_addr8_t VaAddr8
#endif // ub_addr8_t

#ifndef ConvRelu_t
#define ConvRelu_t ConvRelu
#endif // ConvRelu_t

#ifndef vpart_t
#define vpart_t Vpart
#endif // vpart_t

#ifndef __bf16
struct Bf16 {
    char val[2];
};
#define __bf16 struct Bf16
#endif  // __bf16

#ifndef addr_cal_mode_t
#define addr_cal_mode_t AddrCalMode
#endif // addr_cal_mode_t

#ifndef __simt_callee__
#define __simt_callee__
#endif // __simt_callee__

#include <cstdint>
#endif  // __CCE_IS_AICORE__

#endif  // CCEC_DEFINS_H
