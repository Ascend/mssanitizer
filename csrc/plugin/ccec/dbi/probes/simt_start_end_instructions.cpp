/* -------------------------------------------------------------------------
 * This file is part of the MindStudio project.
 * Copyright (c) 2026 Huawei Technologies Co.,Ltd.
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

#include "core/framework/record_defs.h"

#if defined(BISHENG_SUPPORT_SIMT_CALL_DBI)

#if defined(__NPU_ARCH__) && (__NPU_ARCH__ == 3101 || __NPU_ARCH__ == 3510) && defined(__DAV_VEC__)
#define SIMT_MODE  // 开启simt模式
#include "plugin/record_simt_instructions.h"
using namespace Sanitizer;

SANITIZER_REPORT(simt_start)
{
    SimtRecordEmptyEvent<RecordType::SIMT_START>(EXTRA_PARAMS);
}

SANITIZER_REPORT(simt_end)
{
    SimtRecordEmptyEvent<RecordType::SIMT_END>(EXTRA_PARAMS);
}

#undef SIMT_MODE
#else  // NOT C310
#include "plugin/utils.h"
using namespace Sanitizer;

SANITIZER_REPORT(simt_start) {}

SANITIZER_REPORT(simt_end) {}

#endif

#endif
