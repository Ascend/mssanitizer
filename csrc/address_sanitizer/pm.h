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


#ifndef ADDRESS_SANITIZER_PM_H
#define ADDRESS_SANITIZER_PM_H

#include <cstdint>
#include <vector>

namespace Sanitizer {

// gm建模地址范围0 ~ 0xFFFF FFFF FFFF (48 bits)
constexpr uint64_t GLOBAL_MEM_MASK = 0xFFFFFFFFFFFFULL;
// 片上内存建模地址范围0 ~ 0xF FFFF FFFF (36 bits)
constexpr uint64_t LOCAL_MEM_MASK = 0xFFFFFFFFFULL;

// 用于标记GM上定义的数据来源于host
constexpr uint64_t ONE_SM_STAND_FOR_BYTE = 65536U; // 64KB

class PM;

class Range1D {
public:
    class Iterator {
    public:
        friend class Range1D;
        Iterator &operator++(void);
        Iterator &operator+=(uint64_t n);
        bool operator==(Iterator const &other) const { return addr_ == other.addr_; }
        bool operator!=(Iterator const &other) const { return addr_ != other.addr_; }
        bool operator<(Iterator const &other) const  { return addr_ <  other.addr_; }
        uint8_t GetBits(void) const;
    private:
        explicit Iterator(const Range1D &range, uint64_t addr);
    private:
        const Range1D &range_;
        uint64_t addr_;
    };

    explicit Range1D(PM &pm, uint64_t addr, uint64_t size);
    Iterator Begin(void) const;
    Iterator End(void) const;
    Iterator At(uint64_t addr) const;
    void Set(uint8_t bits);
    uint64_t Size(void) const;
    // 返回该地址后标记一致的内存长度
    uint64_t UnifiedSizeAfter(Iterator const &it) const;
    Range1D UnifiedRangeAfter(Iterator const &it) const;
private:
    PM &pm_;
    uint64_t addr_;
    uint64_t size_;
};

// Primary Map
class PM {
public:
    // Secondary Map(SM)
    // 参考论文"How to Shadow Every Byte of Memory Used by a Program"实现
    // 表示64KB内存状态
    // 每2个va-bits表示一个字节的内存状态，每6个c-bits表示一个字节归属的核ID
    // 6个cbits难以连续访问，因此把每个字节的va-bits与c-bits合并为一个字节，va-bits位于低2位
    struct SM;
    friend class Range1D;
    using SmPtr = SM *;

    explicit PM(const uint64_t &byteNum, uint8_t memInitVal) noexcept;
    virtual void Reset(uint8_t memInitVal) noexcept;
    virtual ~PM();
    Range1D GetRange(uint64_t addr, uint64_t size);
    virtual uint8_t GetBits(uint64_t addr);
    virtual void Set(uint64_t addr, uint64_t size, uint8_t bits);
    virtual uint64_t UnifiedSizeAfter(uint64_t baseAddr, uint64_t size, uint64_t addr) const;
    // 快速判断 [addr, addr+size) 内是否存在状态(低2bit)==status 的字节；无则返回 false
    virtual bool RangeHasStatus(uint64_t addr, uint64_t size, uint8_t status) const;
    // 掩码用于从地址中提取出查表使用的地址
    // 不同芯片的gm建模地址会有差异，因此掩码也会不同

protected:

    std::vector<SmPtr> smList_;
    std::vector<uint8_t> commonBitsList_;
    uint64_t byteNum_;
    uint64_t smNum_;

private:
    // 获取在smList(三级表)中的索引
    inline uint64_t GetBlockIdx(uint64_t addr) const;
    inline uint64_t GetBlockOffset(uint64_t addr) const;
    const uint64_t blockSize_ = ONE_SM_STAND_FOR_BYTE;
};

// PM为shallow memory的一级表结构，用于对内存17~36位的地址进行索引
// GmPM为PM for Global memory的简写。相比于片上内存，Gm有更大的建模范围，目前已知需要对48位地址进行建模。
// 通过增加一级表的方式，在原PM的基础上扩展了高位地址（37~48位）。GmPM继承PM的是为了在shallow_memory代码中统一Gm和片上内存
// 的语义。GmPM内部会根据地址调用PM的方法。
class GmPM : public PM {
public:
    explicit GmPM(uint8_t memInitVal) noexcept;
    void Reset(uint8_t memInitVal) noexcept override;
    ~GmPM() override;
    using PmPtr = PM *;
    uint8_t GetBits(uint64_t addr) override;
    void Set(uint64_t addr, uint64_t size, uint8_t bits) override;
    uint64_t UnifiedSizeAfter(uint64_t baseAddr, uint64_t size, uint64_t addr) const override;
    bool RangeHasStatus(uint64_t addr, uint64_t size, uint8_t status) const override;

private:
    // 获取PM指针，不存在时返回空
    PmPtr QueryPM(uint64_t addr) const;
    // 获取PM指针，不存在时创建
    PmPtr GetPM(uint64_t addr);
    std::vector<PmPtr> pmList_;
    const uint8_t memInitVal_;
    const uint64_t blockSize_ = LOCAL_MEM_MASK;

    // 获取在pmList(二级表)中的索引
    inline uint64_t GetBlockIdx(uint64_t addr) const;
    // 获取在pmList(二级表)中的偏移
    inline uint64_t GetBlockOffset(uint64_t addr) const;
};
}

#endif
