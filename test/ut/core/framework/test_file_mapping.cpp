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


#include <vector>
#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <any>
#include <mutex>

#include "utility/elf_loader.h"
#include "utility/serializer.h"

#define private public
#include "file_mapping.h"
#undef private

using namespace Sanitizer;

inline std::vector<char> ToBuffer(std::string const &str)
{
    return std::vector<char>(str.cbegin(), str.cend());
}

auto GetFileMappingList()
{
    static std::map<std::string, uint64_t> fileMappingList = {
        {"aaa.cpp", 0x123456}, {"bbb.cpp", 0x3212341}, {"ccc.c", 0x1000212}};
    return fileMappingList;
}

inline std::vector<char> SerializeFileMap(void)
{
    auto fileMappingList = GetFileMappingList();
    std::vector<char> fileMapping;
    auto fillFileMapping = [&fileMapping](const std::pair<std::string, uint64_t> &p) {
        fileMapping.insert(fileMapping.end(), p.first.cbegin(), p.first.cend());
        fileMapping.push_back('\0');
        uint64_t uuid = p.second;
        auto strUUID = Serialize(uuid);
        fileMapping.insert(fileMapping.end(), strUUID.cbegin(), strUUID.cend());
    };

    for_each(fileMappingList.begin(), fileMappingList.end(), fillFileMapping);
    return fileMapping;
}

TEST(FileMapping, load_from_buffer_stream_expect_get_empty_map)
{
    FileMapping &fileMapping = FileMapping::Instance();
    std::vector<char> buffer;
    fileMapping.Load(buffer);
    ASSERT_TRUE(fileMapping.fileMap_.empty());
}

TEST(FileMapping, load_from_buffer_with_nuls_expect_get_empty_map)
{
    FileMapping &fileMapping = FileMapping::Instance();
    std::vector<char> buffer(10, '\0');
    fileMapping.Load(buffer);
    ASSERT_TRUE(fileMapping.fileMap_.empty());
}

TEST(FileMapping, load_from_buffer_with_filename_without_nul_expect_get_empty_map)
{
    FileMapping &fileMapping = FileMapping::Instance();
    fileMapping.Load(ToBuffer("test.cpp"));
    ASSERT_TRUE(fileMapping.fileMap_.empty());
}

TEST(FileMapping, load_from_buffer_with_filename_with_incomplete_uuid_expect_get_empty_map)
{
    FileMapping &fileMapping = FileMapping::Instance();
    std::vector<char> buffer(10, '\0');
    std::string file{"test.cpp"};
    buffer.insert(buffer.end(), file.cbegin(), file.cend());
    buffer.emplace_back('\0');
    std::string uuid = Serialize<uint32_t>(123);
    buffer.insert(buffer.end(), uuid.cbegin(), uuid.cend());
    fileMapping.Load(buffer);
    ASSERT_TRUE(fileMapping.fileMap_.empty());
}

std::vector<char> CreateElf64Ehdr()
{
    Elf64_Ehdr st;
    constexpr uint shnum = 2;
    st.e_ident[EI_MAG0] = ELFMAG0;
    st.e_ident[EI_MAG1] = ELFMAG1;
    st.e_ident[EI_MAG2] = ELFMAG2;
    st.e_ident[EI_MAG3] = ELFMAG3;
    st.e_ident[EI_CLASS] = ELFCLASS64;
    st.e_ident[EI_DATA] = ELFDATA2LSB;
    st.e_shnum = shnum;
    st.e_shstrndx = 0;
    st.e_shentsize = sizeof(Elf64_Shdr);
    Elf64_Shdr nameSecHeader{};

    std::string segName0 = ".seg_name";
    std::string segName1 = ".init_array_sanitizer_file_mapping";
    std::vector<char> segNameBuff;
    segNameBuff.insert(segNameBuff.end(), segName0.begin(), segName0.end());
    segNameBuff.push_back('\0');
    segNameBuff.insert(segNameBuff.end(), segName1.begin(), segName1.end());
    segNameBuff.push_back('\0');

    auto fileMapping = SerializeFileMap();
    Elf64_Shdr fileMappingSecHeader;

    size_t bufLen = sizeof(Elf64_Ehdr) + sizeof(Elf64_Shdr) * 2 + segNameBuff.size() + fileMapping.size();
    std::vector<char> buff(bufLen);
    auto strSt = Serialize(st);
    auto buffIt = std::copy_n(strSt.cbegin(), strSt.size(), buff.begin());
    auto headPtr = reinterpret_cast<Elf64_Ehdr *>(buff.data());
    auto nameSecHeadPtr = reinterpret_cast<Elf64_Shdr*>(&(*buffIt));
    auto segAddr = reinterpret_cast<Elf64_Off>(buff.data());
    auto nameSecAddr = reinterpret_cast<Elf64_Off>(nameSecHeadPtr);
    headPtr->e_shoff = nameSecAddr - segAddr;
    auto strNameSecHeader = Serialize(nameSecHeader);
    buffIt = std::copy_n(strNameSecHeader.cbegin(), strNameSecHeader.size(), buffIt);

    auto fileMappingSecHeadPtr = reinterpret_cast<Elf64_Shdr*>(&(*buffIt));
    auto strFileMappingSecHead = Serialize(fileMappingSecHeader);
    buffIt = std::copy_n(strFileMappingSecHead.cbegin(), strFileMappingSecHead.size(), buffIt);
    nameSecHeadPtr->sh_offset = reinterpret_cast<Elf64_Off>(&(*buffIt)) - reinterpret_cast<Elf64_Off>(buff.data());
    nameSecHeadPtr->sh_size = segNameBuff.size();
    nameSecHeadPtr->sh_name = 0;

    fileMappingSecHeadPtr->sh_name = segName0.size() + 1;
    buffIt = std::copy_n(segNameBuff.begin(), segNameBuff.size(), buffIt);
    fileMappingSecHeadPtr->sh_offset =
        reinterpret_cast<Elf64_Off>(&(*buffIt)) - reinterpret_cast<Elf64_Off>(buff.data());
    fileMappingSecHeadPtr->sh_size = fileMapping.size();
    std::copy_n(fileMapping.begin(), fileMapping.size(), buffIt);
    return buff;
}

TEST(FileMapping, load_from_buffer_with_filename_with_complete_uuid_expect_get_file_map)
{
    FileMapping &fileMapping = FileMapping::Instance();
    auto buffer = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>(CreateElf64Ehdr()));
    auto loader = Sanitizer::ElfLoader();
    ASSERT_TRUE(loader.FromBuffer(buffer));
    Sanitizer::Elf elf = loader.Load();
    std::vector<char> fileMappingSeg = elf.ReadRawData(".init_array_sanitizer_file_mapping");
    fileMapping.Load(fileMappingSeg);
    auto expectFileMapping = GetFileMappingList();
    ASSERT_EQ(fileMapping.fileMap_.size(), expectFileMapping.size());
    for (const auto &p : expectFileMapping) {
        ASSERT_EQ(fileMapping.fileMap_[p.second].fileName, p.first);
    }
}

TEST(ElfLoaderDetail, parse_elf_with_too_small_or_bad_buffer_expect_fail)
{
    detail::ElfSections sections;
    std::vector<char> tiny(4, 0);
    auto tinyBuffer = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>(tiny));
    ASSERT_FALSE(detail::ParseElf(*tinyBuffer, sections));

    // 大小合法但没有 ELF magic
    std::vector<char> garbage(1024, 0);
    auto garbageBuffer = BinaryBuffer::FromVector(std::make_shared<const std::vector<char>>(garbage));
    ASSERT_FALSE(detail::ParseElf(*garbageBuffer, sections));
}
