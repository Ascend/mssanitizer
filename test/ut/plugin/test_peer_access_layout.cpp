/* Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved. */
#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include "ccec_defs.h"
#include "plugin/online_check.h"
#include "plugin/recorder.h"
#include "peer_host_layout_fixture.h"
#include "kernel_block.h"
#include <cstring>

using namespace Sanitizer;
TEST(PeerOnlineLayout, HostProtocolWrittenBufferIsConsumedByRealPluginInEveryBlock) {
    ASSERT_EQ(PeerHostGlobalSize(), sizeof(RecordGlobalHead));
    // 远端扩展位于原参数寄存器之后，不移动既有字段
    EXPECT_GE(offsetof(RecordBlockHeadImpl, peerMemoryNum),
        offsetof(RecordBlockHeadImpl, paraBase) + sizeof(ParaBaseRegister));
    ASSERT_EQ(PeerHostBlockSize(), sizeof(RecordBlockHead));
    ASSERT_EQ(PeerHostLocalSize(), sizeof(HostMemoryInfo));
    ASSERT_EQ(PeerHostRangeSize(), sizeof(PeerMemoryInfo));
    for (uint32_t locals : {0U, 1U, 3U}) {
        for (uint32_t peers : {0U, 1U, 17U}) {
            for (uint32_t parameters : {0U, 1U, 7U}) {
                SCOPED_TRACE(std::to_string(locals) + "/" + std::to_string(peers) + "/" + std::to_string(parameters));
                const size_t size = PeerHostBufferSize(locals, peers, 3);
                std::vector<uint8_t> bytes(size + 64, 0);
                std::fill(bytes.begin() + size, bytes.end(), 0xa5);
                PeerWriteHostBuffer(bytes.data(), locals, peers, 3, parameters);
                auto *global = reinterpret_cast<RecordGlobalHead *>(bytes.data());
                ASSERT_EQ(global->kernelInfo.kernelParamNum, parameters);
                ASSERT_EQ(global->offsetInfo.blockHeadSize, GetRecordHeadSize(locals, peers));
                const size_t stride = global->offsetInfo.blockHeadSize + 16384;
                for (size_t b = 0; b < 3; ++b) {
                    uint8_t *base = bytes.data() + sizeof(RecordGlobalHead) + b * stride;
                    auto *block = reinterpret_cast<RecordBlockHead *>(base);
                    ASSERT_EQ(block->hostMemoryNum, locals);
                    ASSERT_EQ(block->peerMemoryNum, peers);
                    ASSERT_EQ(block->peerMemoryInfoPtr,
                        peers ? reinterpret_cast<PeerMemoryInfo *>(
                                    base + sizeof(RecordBlockHead) + locals * sizeof(HostMemoryInfo))
                              : nullptr);
                    for (uint32_t i = 0; i < peers; ++i) {
                        EXPECT_EQ(block->peerMemoryInfoPtr[i].deviceId, 1 + i % 3);
                        EXPECT_EQ(block->peerMemoryInfoPtr[i].addr, 0x8000 + i * 128ULL);
                        EXPECT_EQ(block->peerMemoryInfoPtr[i].size, 64);
                        EXPECT_EQ(block->peerMemoryInfoPtr[i].permission, 3);
                    }
                    uint8_t *simt = base + global->offsetInfo.simtErrorInfo.offset;
                    if (peers) {
                        ASSERT_GE(simt,
                            reinterpret_cast<uint8_t *>(block->peerMemoryInfoPtr) + peers * sizeof(PeerMemoryInfo));
                    }
                    OnlineCheck check;
                    check.Init(bytes.data(), simt, base, b);
                    SimtLoadStoreRecord record{};
                    record.space = AddressSpace::GM;
                    record.size = 4;
                    record.addr = peers ? 0x8000 + (peers - 1) * 128ULL : 0xdead0000;
                    check.Process<RecordType::SIMT_STG>(record);
                    auto *head = reinterpret_cast<SimtRecordBlockHead *>(simt);
                    EXPECT_EQ(head->writeOffset == 0, peers != 0);
                    EXPECT_EQ(base[stride - 1], 0);
                    if (b + 1 < 3) {
                        auto *next = reinterpret_cast<RecordBlockHead *>(base + stride);
                        EXPECT_EQ(next->peerMemoryNum, peers);
                    }
                }
                EXPECT_TRUE(std::all_of(bytes.begin() + size, bytes.end(), [](uint8_t v) { return v == 0xa5; }));
            }
        }
    }
}
