/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved. */
#include <cstring>
#include "peer_host_layout_fixture.h"
#include "utils/Protocol.h"

namespace {
size_t HeadSize(uint32_t locals, uint32_t peers) {
    return (sizeof(RecordBlockHead) + size_t(locals) * sizeof(HostMemoryInfo) + size_t(peers) * sizeof(PeerMemoryInfo) +
               63) /
        64 * 64;
}
constexpr size_t CACHE = 16384;
}
size_t PeerHostBufferSize(uint32_t locals, uint32_t peers, uint32_t blocks) {
    return sizeof(RecordGlobalHead) + blocks * (HeadSize(locals, peers) + CACHE);
}
size_t PeerHostGlobalSize() { return sizeof(RecordGlobalHead); }
size_t PeerHostBlockSize() { return sizeof(RecordBlockHead); }
size_t PeerHostLocalSize() { return sizeof(HostMemoryInfo); }
size_t PeerHostRangeSize() { return sizeof(PeerMemoryInfo); }
void PeerWriteHostBuffer(
    uint8_t *bytes, uint32_t locals, uint32_t peers, uint32_t blocks, uint32_t parameters, bool supportSimt) {
    RecordGlobalHead global{};
    global.supportSimt = supportSimt;
    global.checkParms.defaultcheck = true;
    global.checkParms.memcheck = true;
    global.kernelInfo.kernelParamNum = parameters;
    global.kernelInfo.totalBlockDim = blocks;
    global.offsetInfo.blockHeadSize = HeadSize(locals, peers);
    global.offsetInfo.simtErrorInfo.offset = HeadSize(locals, peers);
    global.offsetInfo.simtErrorInfo.size = 4096;
    std::memcpy(bytes, &global, sizeof(global));
    for (uint32_t b = 0; b < blocks; ++b) {
        uint8_t *base = bytes + sizeof(global) + b * (HeadSize(locals, peers) + CACHE);
        RecordBlockHead block{};
        block.hostMemoryNum = locals;
        block.peerMemoryNum = peers;
        block.hostMemoryInfoPtr = locals ? reinterpret_cast<HostMemoryInfo *>(base + sizeof(block)) : nullptr;
        block.peerMemoryInfoPtr = peers
            ? reinterpret_cast<PeerMemoryInfo *>(base + sizeof(block) + locals * sizeof(HostMemoryInfo))
            : nullptr;
        std::memcpy(base, &block, sizeof(block));
        for (uint32_t i = 0; i < locals; ++i) {
            block.hostMemoryInfoPtr[i] = {0x1000 + i * 128ULL, 64, 3};
        }
        for (uint32_t i = 0; i < peers; ++i) {
            block.peerMemoryInfoPtr[i] = {0x8000 + i * 128ULL, 64, 3, static_cast<int32_t>(1 + i % 3)};
        }
    }
}
