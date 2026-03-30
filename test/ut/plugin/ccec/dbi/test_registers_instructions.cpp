#include <gtest/gtest.h>
#define BUILD_DYNAMIC_PROBE

#include "../../ccec_defs.h"
#include "../data_process.h"
#ifdef __NPU_ARCH__
#define __NPU_ARCH__ 2201
#include "plugin/ccec/dbi/probes/registers_instructions.cpp"

using namespace Sanitizer;

namespace SanitizerTest {

constexpr uint64_t MEM_INFO_SIZE = 1024 * 1024 * 10;

TEST(RegisterInstructions, set_vector_mask_0_sw_off)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = false;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_vector_mask(memInfo.data(), record.location.pc, 0, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 0);
}

TEST(RegisterInstructions, set_vector_mask_0_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_vector_mask(memInfo.data(), record.location.pc, 0, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_VECTOR_MASK_0>(ptr, record));
}

TEST(RegisterInstructions, set_vector_mask_1_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_vector_mask(memInfo.data(), record.location.pc, 0, 1, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_VECTOR_MASK_1>(ptr, record));
}

TEST(RegisterInstructions, set_ctrl_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_ctrl(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_CTRL>(ptr, record));
}

TEST(RegisterInstructions, set_ffts_base_addr_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_ffts_base_addr(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FFTS_BASE_ADDR>(ptr, record));
}

TEST(RegisterInstructions, set_fpc_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_fpc(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_FPC>(ptr, record));
}

TEST(RegisterInstructions, set_quant_pre_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_quant_pre(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_QUANT_PRE>(ptr, record));
}

TEST(RegisterInstructions, set_quant_post_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    std::vector<uint8_t> memInfo(MEM_INFO_SIZE, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_quant_post(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_QUANT_POST>(ptr, record));
}

TEST(RegisterInstructions, set_lrelu_alpha_expect_get_correct_records)
{
    RegisterSetRecord record{};
    uint64_t regvalU64 = 123456;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_UINT64;
    record.regPayLoad.regVal = regvalU64;

    // uint64
    std::vector<uint8_t> memInfo(MEM_INFO_SIZE * 10, 0);
    RecordGlobalHead head{};
    head.checkParms.registerCheck = true;
    std::copy_n(reinterpret_cast<uint8_t const*>(&head), sizeof(RecordGlobalHead), memInfo.begin());
    __sanitizer_report_set_lrelu_alpha(memInfo.data(), record.location.pc, 0, regvalU64);

    RecordBlockHead blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 1);
    uint8_t *ptr = memInfo.data() + sizeof(RecordGlobalHead) + sizeof(RecordBlockHead);
    auto x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_LRELU_ALPHA>(ptr, record));

    // 测试通过 copy_matrix 指令设置 lrelu_alpha 的数据类型
    __sanitizer_report_copy_matrix_cc_to_cbuf_f32(memInfo.data(), record.location.pc, 0, 0, 0, 0, 0, true);
    record.regPayLoad.regValType = RegisterValueType::VAL_FLOAT;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_LRELU_ALPHA>(ptr, record));

    __sanitizer_report_copy_matrix_cc_to_cbuf_f32(memInfo.data(), record.location.pc, 0, 0, 0, 0, 0, false);
    record.regPayLoad.regValType = RegisterValueType::VAL_INT32;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_LRELU_ALPHA>(ptr, record));

    // half
    half regValHalf = {{'1', '4'}};
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_HALF;
    record.regPayLoad.regVal = 0;
    record.regPayLoad.regVal = *(static_cast<uint16_t *>(static_cast<void *>(&regValHalf)));

    __sanitizer_report_set_lrelu_alpha(memInfo.data(), record.location.pc, 0, regValHalf);

    blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 2);
    ptr += sizeof(RecordType) + sizeof(RegisterSetRecord);
    x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_LRELU_ALPHA>(ptr, record));

    // float
    float regValFloat = 3.14159;
    record.location = {10, 10};
    record.regPayLoad.regValType = RegisterValueType::VAL_FLOAT;
    record.regPayLoad.regVal = 0;
    record.regPayLoad.regVal = *(static_cast<uint32_t *>(static_cast<void *>(&regValFloat)));

    __sanitizer_report_set_lrelu_alpha(memInfo.data(), record.location.pc, 0, regValFloat);

    blockHead = *reinterpret_cast<RecordBlockHead const *>(memInfo.data() + sizeof(RecordGlobalHead));
    ASSERT_EQ(blockHead.recordWriteCount, 3);
    ptr += sizeof(RecordType) + sizeof(RegisterSetRecord);
    x = reinterpret_cast<Sanitizer::RegisterSetRecord *>(ptr + sizeof(RecordType));
    record.location = x->location;
    ASSERT_TRUE(CheckRecordEqual<RecordType::SET_LRELU_ALPHA>(ptr, record));
}

}
#endif