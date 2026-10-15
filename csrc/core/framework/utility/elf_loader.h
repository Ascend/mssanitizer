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


#ifndef CORE_FRAMEWORK_ELF_LOADER_H
#define CORE_FRAMEWORK_ELF_LOADER_H

#include <elf.h>

#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/framework/binary_buffer.h"
#include "log.h"
#include "ustring.h"

namespace Sanitizer {

namespace detail {
constexpr uint64_t MAX_LEGAL_ELF_SEGMENT_SIZE = 16ULL * 1024 * 1024 * 1024;  // 16GB
/**
 * @brief 对读入的ELF文件头做基本校验，保证正常读取映射信息
 *
 * @param ELF文件头
 * @return 校验是否通过
 */
inline bool ValidateElfHeader(const Elf64_Ehdr &elfHeader)
{
    if (elfHeader.e_ident[EI_MAG0] != ELFMAG0 || elfHeader.e_ident[EI_MAG1] != ELFMAG1 ||
        elfHeader.e_ident[EI_MAG2] != ELFMAG2 || elfHeader.e_ident[EI_MAG3] != ELFMAG3) {
        // magic不对，非elf格式文件
        SAN_INFO_LOG("Invalid ELF magic, this is not a elf file.");
        return false;
    }
    if (elfHeader.e_ident[EI_CLASS] != ELFCLASS64) {
        // 非64位elf文件，不支持
        SAN_INFO_LOG("32bit executable is not supported.");
        return false;
    }
    if (elfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
        // 非小端序elf文件，不支持
        SAN_INFO_LOG("Big endian executable is not supported.");
        return false;
    }
    if (elfHeader.e_shnum <= elfHeader.e_shstrndx) {
        // 特殊section序号(.shstrtab，内部保存各个section的名字)超过section个数
        SAN_INFO_LOG("Invalid Elf file.");
        return false;
    }
    return true;
}

/**
 * @brief 从buffer内的特定偏移处读取一个值
 *
 * @tparam 需要读取的类型
 * @param buffer
 * @param 偏移位置
 * @return 读取到的值
 */
template <typename T>
bool ReadValueFromBuffer(const BinaryBuffer &buffer, std::size_t offset, T &value)
{
    static_assert(std::is_standard_layout<T>::value, "T is not standard layout.");
    if (buffer.size() < offset + sizeof(T)) {
        SAN_INFO_LOG("Read out of bound, data [0, %zu), read [%zu, %zu).", buffer.size(), offset, offset + sizeof(T));
        return false;
    }
    const char *base = buffer.data() + offset;
    std::copy(base, base + sizeof(T), static_cast<char *>(static_cast<void *>(&value)));
    return true;
}

/**
 * @brief 从buffer内的特定偏移处读取若干个连续的T
 *
 * @param buffer
 * @param 偏移位置
 * @param 读取个数
 * @return 读取到的值组成的vector，失败时返回空vector
 */
template <typename T>
bool ReadArrayFromBuffer(const BinaryBuffer &buffer, std::size_t offset, unsigned int count,
    std::vector<T> &values)
{
    static_assert(std::is_standard_layout<T>::value, "T is not standard layout.");
    if (count > MAX_LEGAL_ELF_SEGMENT_SIZE) {
        SAN_ERROR_LOG("Elf segment size (%u) exceeds the limit (%lu).", count, MAX_LEGAL_ELF_SEGMENT_SIZE);
        return false;
    }
    values.resize(count);
    if (buffer.size() < offset + count * sizeof(T)) {
        SAN_INFO_LOG("Read out of bound, data [0, %zu), read [%zu, %zu).", buffer.size(), offset,
            offset + count * sizeof(T));
        return false;
    }
    const char *base = buffer.data() + offset;
    std::copy(base, base + count * sizeof(T), values.data());
    return true;
}

/**
 * @brief ELF 解析结果：文件头、节头表与节名字表内容。
 */
struct ElfSections {
    Elf64_Ehdr ehdr{};
    std::vector<Elf64_Shdr> shdrs; // 节头表副本
    uint64_t shTableOff{};
    uint64_t shTableSize{};
    std::vector<char> shstrtab; // 节名字表内容
};

/**
 * @brief 解析 ELF buffer，读取文件头、节头表与节名字表。只读取小数据，不拷贝大段。
 *
 * @param ELF 二进制 buffer
 * @param 输出解析结果
 * @return 解析是否成功
 */
inline bool ParseElf(const BinaryBuffer &buffer, ElfSections &sections) {
    if (buffer.size() < sizeof(Elf64_Ehdr) || buffer.size() > MAX_LEGAL_ELF_SEGMENT_SIZE) {
        return false;
    }
    if (!ReadValueFromBuffer<Elf64_Ehdr>(buffer, 0, sections.ehdr)) {
        return false;
    }
    if (!ValidateElfHeader(sections.ehdr)) {
        return false;
    }
    if (sections.ehdr.e_shoff == 0 || sections.ehdr.e_shentsize != sizeof(Elf64_Shdr)) {
        return false;
    }
    sections.shTableOff = sections.ehdr.e_shoff;
    sections.shTableSize = static_cast<uint64_t>(sections.ehdr.e_shnum) * sections.ehdr.e_shentsize;
    if (sections.shTableOff > buffer.size() || sections.shTableSize > buffer.size() - sections.shTableOff) {
        return false;
    }
    sections.shdrs.resize(sections.ehdr.e_shnum);
    for (uint64_t i = 0; i < sections.ehdr.e_shnum; ++i) {
        if (!ReadValueFromBuffer<Elf64_Shdr>(
                buffer, sections.shTableOff + i * sections.ehdr.e_shentsize, sections.shdrs[i])) {
            return false;
        }
    }
    const Elf64_Shdr &nameSec = sections.shdrs[sections.ehdr.e_shstrndx];
    if (nameSec.sh_size == 0 || nameSec.sh_size > buffer.size() ||
        nameSec.sh_offset > buffer.size() - nameSec.sh_size ||
        nameSec.sh_size > std::numeric_limits<unsigned int>::max()) {
        return false;
    }
    if (!ReadArrayFromBuffer<char>(
            buffer, nameSec.sh_offset, static_cast<unsigned int>(nameSec.sh_size), sections.shstrtab)) {
        return false;
    }
    return true;
}

} // namespace detail

/**
 * @brief Elf类表示一个Elf文件内容，我们可以从中获取某个section的原始数据，
 * 或者如果某个section是StringTable，我们可以直接读取这个StringTable。
 * 为了方便进行错误处理，我们不直接使用Elf类进行ELF文件的读取，而采取builder模式
 * 由下面的ElfLoader类进行实际的读取和校验。
 */
class Elf {
public:
    friend class ElfLoader;
    /**
     * @brief 从Elf对象内读取某个section的原始数据
     *
     * @param section名称
     * @return 该section的原始数据
     */
    std::vector<char> ReadRawData(const std::string &name)
    {
        auto iter = sections_.find(name);
        if (iter == sections_.end()) {
            SAN_INFO_LOG("No section %s found.", name.c_str());
            return {};
        }
        std::vector<char> values;
        detail::ReadArrayFromBuffer<char>(*buffer_, iter->second.sh_offset, iter->second.sh_size, values);
        return values;
    }

    /**
     * @brief 从Elf对象内读取某个section的StringTable
     *
     * @desc StringTable是一种在elf内常用的字符串保存格式
     * 其中的数据是若干以'\0'结尾的字符串拼接(CStr)
     * 其标准保证其内容的第一个及最后一个字节都是'\0'
     *
     * @param section名称
     * @return 该section的StringTable
     */
    std::vector<std::string> ReadStringTable(const std::string &name)
    {
        auto iter = sections_.find(name);
        if (iter == sections_.end()) {
            SAN_INFO_LOG("No section %s found.", name.c_str());
            return {};
        }
        auto &header = iter->second;
        // 目前编译器那边还不知道怎么设置section类型，因此这里添加一个额外的判定
        if (header.sh_offset >= buffer_->size() || buffer_->data()[header.sh_offset] != 0) {
            SAN_INFO_LOG("Section %s is not a valid StringTable.", name.c_str());
            return {};
        }
        if (!(header.sh_size > 2 && buffer_->size() - header.sh_offset > 2)) {
            SAN_INFO_LOG("Section %s has invalid data for a StringTable: sh_size(%zu), available buffer(%zu)",
                name.c_str(), header.sh_size, buffer_->size() - header.sh_offset);
            return {};
        }

        std::vector<std::string> result;
        std::string section_data = std::string(buffer_->data() + header.sh_offset + 1,
            std::min(header.sh_size - 2, buffer_->size() - header.sh_offset - 2));
        Utility::Split(section_data, std::back_inserter(result), std::string(1, '\0'));
        return result;
    }

    auto GetSectionHeaders(void) const -> std::map<std::string, Elf64_Shdr> const &
    {
        return this->sections_;
    }

private:
    Elf(std::map<std::string, Elf64_Shdr> sections, BinaryBuffer::Ptr buffer)
        : buffer_(std::move(buffer)), sections_(std::move(sections))
    {
    }

    // 以 shared_ptr 共享底层二进制（vector 或 mmap'd fd），避免 GB 级 kernel 的全量拷贝
    BinaryBuffer::Ptr buffer_;
    std::map<std::string, Elf64_Shdr> sections_;
};

/**
 * @brief 这个类负责加载ELF文件，如果加载成功则通过Load接口返回已加载的Elf对象。
 * 目前有两种加载方式，FromPath和FromBuffer，这是因为昇腾的elf文件内部嵌套了device侧
 * 的elf文件。
 */
class ElfLoader {
public:
    /**
     * @brief 试图从buffer构造Elf对象
     *
     * @param buffer
     * @return 构造是否成功，成功之后可以使用.Load获取构造的Elf对象
     */
    // 零拷贝入口：直接共享调用方持有的二进制视图（vector 或 mmap'd fd）
    bool FromBuffer(BinaryBuffer::Ptr buffer)
    {
        if (buffer == nullptr || buffer->empty()) {
            SAN_INFO_LOG("ELF contains no data.");
            return false;
        }
        buffer_ = std::move(buffer);
        // 读取文件头并进行简单校验
        if (!detail::ReadValueFromBuffer<Elf64_Ehdr>(*buffer_, 0, header_)) {
            return false;
        }
        if (!detail::ValidateElfHeader(header_)) {
            return false;
        }
        // 各个section的名字保存在一个类型StringTable的section内(一般为.shstrtab)
        // ELF头部的e_shstrndx字段为这个section的序号，通过序号我们可以找到这个section头部
        Elf64_Shdr nameSecHeader {};
        if (!detail::ReadValueFromBuffer<Elf64_Shdr>(*buffer_,
            header_.e_shoff + static_cast<Elf64_Off>(header_.e_shstrndx) * header_.e_shentsize, nameSecHeader)) {
            return false;
        }
        std::vector<char> nameBuffer;
        if (!detail::ReadArrayFromBuffer<char>(*buffer_, nameSecHeader.sh_offset, nameSecHeader.sh_size, nameBuffer)) {
            return false;
        }
        for (unsigned long idx = 0; idx < header_.e_shnum; idx++) {
            // 通过elf头部信息，读取各个section的头部
            // 各个section头部内有一个sh_name字段指向其名字在StringTable里的起点
            Elf64_Shdr sectionHeader {};
            if (!detail::ReadValueFromBuffer<Elf64_Shdr>(*buffer_, header_.e_shoff + idx * header_.e_shentsize,
                sectionHeader)) {
                return false;
            }
            if (sectionHeader.sh_name >= nameBuffer.size() || nameBuffer.data()[nameBuffer.size() - 1] != '\0') {
                SAN_INFO_LOG(
                    "section name offset %u out of bounds or nameBuffer not null-terminated.", sectionHeader.sh_name);
                return false;
            }
            std::string sectionName = std::string{nameBuffer.data() + sectionHeader.sh_name};
            sections_[sectionName] = sectionHeader;
        }
        return true;
    }

    Elf Load() const { return Elf{sections_, buffer_}; }

private:
    Elf64_Ehdr header_{};
    BinaryBuffer::Ptr buffer_;
    std::map<std::string, Elf64_Shdr> sections_;
};

} // namespace Sanitizer

#endif // !CORE_FRAMEWORK_ELF_LOADER_H
