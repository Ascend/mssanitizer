// -------------------------------------------------------------------------
// This file is part of the MindStudio project.
// Copyright (c) 2026 Huawei Technologies Co.,Ltd.
//
// MindStudio is licensed under Mulan PSL v2.
// You can use this software according to the terms and conditions of the Mulan PSL v2.
// You may obtain a copy of Mulan PSL v2 at:
//
//          http://license.coscl.org.cn/MulanPSL2
//
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PSL v2 for more details.
// -------------------------------------------------------------------------

#include "plugin/record_move_instructions.h"

using namespace Sanitizer;

SANITIZER_REPORT(img2colv2_cbuf_to_ca_b8, __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B8>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}

SANITIZER_REPORT(img2colv2_cbuf_to_ca_b16, __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B16>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}

SANITIZER_REPORT(img2colv2_cbuf_to_ca_b32, __ca__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0A, DataType::DATA_B32>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}

SANITIZER_REPORT(img2colv2_cbuf_to_cb_b8, __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B8>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}

SANITIZER_REPORT(img2colv2_cbuf_to_cb_b16, __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B16>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}

SANITIZER_REPORT(img2colv2_cbuf_to_cb_b32, __cb__ void *dst, __cbuf__ void *src, uint64_t config0, uint64_t config1)
{
    RecordLoad3DV2Event<MemType::L1, MemType::L0B, DataType::DATA_B32>(EXTRA_PARAMS, reinterpret_cast<uint64_t>(dst),
                                                reinterpret_cast<uint64_t>(src), config0, config1);
}
