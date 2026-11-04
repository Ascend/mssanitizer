/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved. */
#pragma once
#include <cstddef>
#include <cstdint>
// 独立 TU 使用 msopscommon 的真实协议，避免两端全局模板/枚举在同一 TU 重定义。
size_t PeerHostBufferSize(uint32_t locals, uint32_t peers, uint32_t blocks);
void PeerWriteHostBuffer(
    uint8_t *bytes, uint32_t locals, uint32_t peers, uint32_t blocks, uint32_t parameters, bool supportSimt = true);
size_t PeerHostGlobalSize();
size_t PeerHostBlockSize();
size_t PeerHostLocalSize();
size_t PeerHostRangeSize();
