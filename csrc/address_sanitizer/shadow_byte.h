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

#ifndef ADDRESS_SANITIZER_SHADOW_BYTE_H
#define ADDRESS_SANITIZER_SHADOW_BYTE_H

#include <cstdint>

namespace Sanitizer {

enum class MemStatus : uint8_t {
    NOACCESS = 0, // unaddressable
    UNDEFINED,    // addressable but uninitialized
    DEFINED,      // addressable and initialized
    ERROR,        // 多核写GM内存，内存被踩后置为该状态，该状态不参与action操作
};

// GM 影子每字节：低 2 位为 MemStatus，高 6 位为该字节归属的核 ID。
constexpr uint8_t VA_BITS_SHIFT = 2;
constexpr uint8_t STATUS_MASK = 0b11;
constexpr uint8_t COREID_MASK = static_cast<uint8_t>(~0b11);
constexpr uint8_t DEFAULT_CORE_ID = 0b111111;

inline uint8_t MakeByte(MemStatus status, uint8_t coreId)
{
    return (static_cast<uint8_t>(status) & STATUS_MASK) |
        ((coreId << VA_BITS_SHIFT) & COREID_MASK);
}

inline MemStatus GetMemStatus(uint8_t byte)
{
    return static_cast<MemStatus>(byte & STATUS_MASK);
}

inline uint8_t GetCoreId(uint8_t byte)
{
    return (byte & COREID_MASK) >> VA_BITS_SHIFT;
}

inline uint8_t SetMemStatus(uint8_t byte, MemStatus status)
{
    return (byte & COREID_MASK) | (static_cast<uint8_t>(status) & STATUS_MASK);
}

inline uint8_t SetCoreId(uint8_t byte, uint8_t coreId)
{
    return (byte & STATUS_MASK) | ((coreId << VA_BITS_SHIFT) & COREID_MASK);
}

// 归还跨 kernel 归属：coreId 复位为默认值；因踩踏被冻结的 ERROR 解冻为 DEFINED（该字节刚被写过）。
// DEFINED/UNDEFINED/NOACCESS 保持不变，避免影响初始化状态检测。
inline uint8_t ReleaseOwnerBitsOfByte(uint8_t byte)
{
    if (GetMemStatus(byte) == MemStatus::ERROR) {
        byte = SetMemStatus(byte, MemStatus::DEFINED);
    }
    return SetCoreId(byte, DEFAULT_CORE_ID);
}

} // namespace Sanitizer

#endif
