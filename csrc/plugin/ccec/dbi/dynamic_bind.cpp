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
#include "MSBit.h"

extern "C" {
// ccec type, stub function name, stub function args index
struct BindStub {
    InstrType instrType;
    const std::string injectedFuncName;
    const std::vector<uint16_t> paraMask;
};

std::vector<BindStub> bindStubs{

    /// dma_mov_registers.cpp start
    {InstrType::PAD_CNT_NDDMA, "__sanitizer_report_set_pad_cnt_nddma", {0}},
    {InstrType::LOOP0_STRIDE_NDDMA, "__sanitizer_report_set_loop0_stride_nddma", {0}},
    {InstrType::LOOP1_STRIDE_NDDMA, "__sanitizer_report_set_loop1_stride_nddma", {0}},
    {InstrType::LOOP2_STRIDE_NDDMA, "__sanitizer_report_set_loop2_stride_nddma", {0}},
    {InstrType::LOOP3_STRIDE_NDDMA, "__sanitizer_report_set_loop3_stride_nddma", {0}},
    {InstrType::LOOP4_STRIDE_NDDMA, "__sanitizer_report_set_loop4_stride_nddma", {0}},
    {InstrType::SET_MTE2_NZ_PARA, "__sanitizer_report_set_mte2_nz_para", {0}},
    /// dma_mov_registers.cpp end

    /// dma_mov.cpp start
    {InstrType::COPY_GM_TO_UBUF, "__sanitizer_report_copy_gm_to_ubuf", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_GM, "__sanitizer_report_copy_ubuf_to_gm", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_GM_BYTE, "__sanitizer_report_copy_ubuf_to_gm_byte", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF, "__sanitizer_report_copy_gm_to_cbuf", {0, 1, 2, 3}},
    {InstrType::COPY_CBUF_TO_GM, "__sanitizer_report_copy_cbuf_to_gm", {0, 1, 2}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B8, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B16, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_B32S, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_b32s", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_V2, "__sanitizer_report_copy_gm_to_cbuf_v2", {0, 1, 2, 3}},
    {InstrType::NDDMA_OUT_TO_UB_B8, "__sanitizer_report_nd_copy_gm_to_ubuf_b8", {0, 1, 2, 3}},
    {InstrType::NDDMA_OUT_TO_UB_B16, "__sanitizer_report_nd_copy_gm_to_ubuf_b16", {0, 1, 2, 3}},
    {InstrType::NDDMA_OUT_TO_UB_B32, "__sanitizer_report_nd_copy_gm_to_ubuf_b32", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B8, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B16, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b16",
        {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_ND2NZ_D_B32, "__sanitizer_report_copy_gm_to_cbuf_multi_nd2nz_d_b32",
        {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B8, "__sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B16, "__sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b16",
        {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_MULTI_DN2NZ_D_B32, "__sanitizer_report_copy_gm_to_cbuf_multi_dn2nz_d_b32",
        {0, 1, 2, 3}},
    {InstrType::SET_L1_2D_B16, "__sanitizer_report_set_l1_2d_b16", {0, 1}},
    {InstrType::SET_L1_2D_B32, "__sanitizer_report_set_l1_2d_b32", {0, 1}},
    {InstrType::COPY_CBUF_TO_UBUF, "__sanitizer_report_copy_cbuf_to_ubuf", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_CBUF, "__sanitizer_report_copy_ubuf_to_cbuf", {0, 1, 2}},
    {InstrType::COPY_UBUF_TO_UBUF, "__sanitizer_report_copy_ubuf_to_ubuf", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_BT_F32, "__sanitizer_report_copy_cbuf_to_bt_f32", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_BT_S32, "__sanitizer_report_copy_cbuf_to_bt_s32", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_BT_F16, "__sanitizer_report_copy_cbuf_to_bt_f16", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_BT_B16, "__sanitizer_report_copy_cbuf_to_bt_b16", {0, 1, 2}},
    {InstrType::COPY_CBUF_TO_FBUF, "__sanitizer_report_copy_cbuf_to_fbuf", {0, 1, 2}},
    /// dma_mov.cpp end

    /// mov_align_registers.cpp start
    {InstrType::LOOP_SIZE_UBTOOUT, "__sanitizer_report_set_loop_size_ubtoout", {0}},
    {InstrType::LOOP1_STRIDE_UBTOOUT, "__sanitizer_report_set_loop1_stride_ubtoout", {0}},
    {InstrType::LOOP2_STRIDE_UBTOOUT, "__sanitizer_report_set_loop2_stride_ubtoout", {0}},
    {InstrType::LOOP_SIZE_OUTTOUB, "__sanitizer_report_set_loop_size_outtoub", {0}},
    {InstrType::LOOP1_STRIDE_OUTTOUB, "__sanitizer_report_set_loop1_stride_outtoub", {0}},
    {InstrType::LOOP2_STRIDE_OUTTOUB, "__sanitizer_report_set_loop2_stride_outtoub", {0}},
    {InstrType::SET_LOOP_SIZE_OUTTOL1, "__sanitizer_report_set_loop_size_outtol1", {0}},
    {InstrType::SET_LOOP1_STRIDE_OUTTOL1, "__sanitizer_report_set_loop1_stride_outtol1", {0}},
    {InstrType::SET_LOOP2_STRIDE_OUTTOL1, "__sanitizer_report_set_loop2_stride_outtol1", {0}},
    /// mov_align_registers.cpp end

    /// mov_align.cpp start
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B16, "__sanitizer_report_copy_gm_to_ubuf_align_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B32, "__sanitizer_report_copy_gm_to_ubuf_align_b32", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_B8, "__sanitizer_report_copy_gm_to_ubuf_align_b8", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B32, "__sanitizer_report_copy_ubuf_to_gm_align_b32", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B16, "__sanitizer_report_copy_ubuf_to_gm_align_b16", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_B8, "__sanitizer_report_copy_ubuf_to_gm_align_b8", {0, 1, 2, 3}},
    {InstrType::COPY_UBUF_TO_GM_ALIGN_V2, "__sanitizer_report_copy_ubuf_to_gm_align_v2", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B8, "__sanitizer_report_copy_gm_to_ubuf_align_v2_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B16, "__sanitizer_report_copy_gm_to_ubuf_align_v2_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_UBUF_ALIGN_V2_B32, "__sanitizer_report_copy_gm_to_ubuf_align_v2_b32", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B8, "__sanitizer_report_copy_gm_to_cbuf_align_v2_b8", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B16, "__sanitizer_report_copy_gm_to_cbuf_align_v2_b16", {0, 1, 2, 3}},
    {InstrType::COPY_GM_TO_CBUF_ALIGN_V2_B32, "__sanitizer_report_copy_gm_to_cbuf_align_v2_b32", {0, 1, 2, 3}},
    /// mov_align.cpp end

    /// load_2d.cpp start
    {InstrType::LOAD_GM_TO_CA, "__sanitizer_report_load_gm_to_ca", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CB, "__sanitizer_report_load_gm_to_cb", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CBUF, "__sanitizer_report_load_gm_to_cbuf", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B4, "__sanitizer_report_load_cbuf_to_cb_2d_transpose_b4", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B8, "__sanitizer_report_load_cbuf_to_cb_2d_transpose_b8", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B16, "__sanitizer_report_load_cbuf_to_cb_2d_transpose_b16", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_TRANSPOSE_B32, "__sanitizer_report_load_cbuf_to_cb_2d_transpose_b32", {0, 1, 2, 3}},
    {InstrType::LOAD_GM_TO_CBUF_2DV2, "__sanitizer_report_load_gm_to_cbuf_2dv2", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CA_B4, "__sanitizer_report_load_cbuf_to_ca_2dv2_b4", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B8, "__sanitizer_report_load_cbuf_to_ca_2dv2_b8", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B16, "__sanitizer_report_load_cbuf_to_ca_2dv2_b16", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_B32, "__sanitizer_report_load_cbuf_to_ca_2dv2_b32", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B4, "__sanitizer_report_load_cbuf_to_cb_2dv2_b4", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B8, "__sanitizer_report_load_cbuf_to_cb_2dv2_b8", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B16, "__sanitizer_report_load_cbuf_to_cb_2dv2_b16", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CB_B32, "__sanitizer_report_load_cbuf_to_cb_2dv2_b32", {0, 1, 2, 3, 4}},
    {InstrType::LOAD_CBUF_TO_CA_MX, "__sanitizer_report_load_cbuf_to_ca_mx_2dv2", {0, 1, 2, 3}},
    {InstrType::LOAD_CBUF_TO_CB_MX, "__sanitizer_report_load_cbuf_to_cb_mx_2dv2", {0, 1, 2, 3}},
    /// load_2d.cpp end

    /// load_3d.cpp start
    {InstrType::IMG2COLV2_CBUF_TO_CA_B8, "__sanitizer_report_img2colv2_cbuf_to_ca_b8", {0, 1, 2, 3}},
    {InstrType::IMG2COLV2_CBUF_TO_CA_B16, "__sanitizer_report_img2colv2_cbuf_to_ca_b16", {0, 1, 2, 3}},
    {InstrType::IMG2COLV2_CBUF_TO_CA_B32, "__sanitizer_report_img2colv2_cbuf_to_ca_b32", {0, 1, 2, 3}},
    {InstrType::IMG2COLV2_CBUF_TO_CB_B8, "__sanitizer_report_img2colv2_cbuf_to_cb_b8", {0, 1, 2, 3}},
    {InstrType::IMG2COLV2_CBUF_TO_CB_B16, "__sanitizer_report_img2colv2_cbuf_to_cb_b16", {0, 1, 2, 3}},
    {InstrType::IMG2COLV2_CBUF_TO_CB_B32, "__sanitizer_report_img2colv2_cbuf_to_cb_b32", {0, 1, 2, 3}},
    /// load_3d.cpp end

    /// mov_fp.cpp start
    {InstrType::COPY_MATRIX_CC_TO_GM_F32, "__sanitizer_report_copy_matrix_cc_to_gm_f32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_S32, "__sanitizer_report_copy_matrix_cc_to_gm_s32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_F32_A5, "__sanitizer_report_copy_matrix_cc_to_gm_f32_a5", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_GM_S32_A5, "__sanitizer_report_copy_matrix_cc_to_gm_s32_a5", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_CBUF_F32, "__sanitizer_report_copy_matrix_cc_to_cbuf_f32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_CBUF_S32, "__sanitizer_report_copy_matrix_cc_to_cbuf_s32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_UB_F32, "__sanitizer_report_copy_matrix_cc_to_ubuf_f32", {0, 1, 2, 3}},
    {InstrType::COPY_MATRIX_CC_TO_UB_S32, "__sanitizer_report_copy_matrix_cc_to_ubuf_s32", {0, 1, 2, 3}},
    /// mov_fp.cpp end

    /// load_store.cpp start
    {InstrType::ST_B64_IMM, "__sanitizer_report_st_b64_imm", {1, 2, 3}},
    {InstrType::ST_B32_IMM, "__sanitizer_report_st_b32_imm", {1, 2, 3}},
    {InstrType::ST_B16_IMM, "__sanitizer_report_st_b16_imm", {1, 2, 3}},
    {InstrType::ST_B8_IMM, "__sanitizer_report_st_b8_imm", {1, 2, 3}},
    {InstrType::ST_B64_REG, "__sanitizer_report_st_b64_reg", {1, 2, 3}},
    {InstrType::ST_B32_REG, "__sanitizer_report_st_b32_reg", {1, 2, 3}},
    {InstrType::ST_B16_REG, "__sanitizer_report_st_b16_reg", {1, 2, 3}},
    {InstrType::ST_B8_REG, "__sanitizer_report_st_b8_reg", {1, 2, 3}},
    {InstrType::STP_B64, "__sanitizer_report_stp_b64", {2, 3}},
    {InstrType::STP_B32, "__sanitizer_report_stp_b32", {2, 3}},
    {InstrType::STP_B16, "__sanitizer_report_stp_b16", {2, 3}},
    {InstrType::STP_B8, "__sanitizer_report_stp_b8", {2, 3}},
    {InstrType::STI_B64_IMM, "__sanitizer_report_sti_b64_imm", {1, 2, 3}},
    {InstrType::STI_B32_IMM, "__sanitizer_report_sti_b32_imm", {1, 2, 3}},
    {InstrType::STI_B16_IMM, "__sanitizer_report_sti_b16_imm", {1, 2, 3}},
    {InstrType::STI_B8_IMM, "__sanitizer_report_sti_b8_imm", {1, 2, 3}},
    {InstrType::STI_B64_REG, "__sanitizer_report_sti_b64_reg", {1, 2, 3}},
    {InstrType::STI_B32_REG, "__sanitizer_report_sti_b32_reg", {1, 2, 3}},
    {InstrType::STI_B16_REG, "__sanitizer_report_sti_b16_reg", {1, 2, 3}},
    {InstrType::STI_B8_REG, "__sanitizer_report_sti_b8_reg", {1, 2, 3}},
    {InstrType::LD_B64_IMM, "__sanitizer_report_ld_b64_imm", {1, 2, 3}},
    {InstrType::LD_B32_IMM, "__sanitizer_report_ld_b32_imm", {1, 2, 3}},
    {InstrType::LD_B16_IMM, "__sanitizer_report_ld_b16_imm", {1, 2, 3}},
    {InstrType::LD_B8_IMM, "__sanitizer_report_ld_b8_imm", {1, 2, 3}},
    {InstrType::LD_B64_REG, "__sanitizer_report_ld_b64_reg", {1, 2, 3}},
    {InstrType::LD_B32_REG, "__sanitizer_report_ld_b32_reg", {1, 2, 3}},
    {InstrType::LD_B16_REG, "__sanitizer_report_ld_b16_reg", {1, 2, 3}},
    {InstrType::LD_B8_REG, "__sanitizer_report_ld_b8_reg", {1, 2, 3}},
    {InstrType::LDP_B64, "__sanitizer_report_ldp_b64", {2, 3}},
    {InstrType::LDP_B32, "__sanitizer_report_ldp_b32", {2, 3}},
    {InstrType::LDP_B16, "__sanitizer_report_ldp_b16", {2, 3}},
    {InstrType::LDP_B8, "__sanitizer_report_ldp_b8", {2, 3}},
    {InstrType::ST_ATOMIC_B32, "__sanitizer_report_st_atomic_b32", {1, 2, 3}},
    {InstrType::ST_ATOMIC_B16, "__sanitizer_report_st_atomic_b16", {1, 2, 3}},
    {InstrType::ST_ATOMIC_B8, "__sanitizer_report_st_atomic_b8", {1, 2, 3}},
    {InstrType::STI_ATOMIC_B32, "__sanitizer_report_sti_atomic_b32", {1, 2, 3}},
    {InstrType::STI_ATOMIC_B16, "__sanitizer_report_sti_atomic_b16", {1, 2, 3}},
    {InstrType::STI_ATOMIC_B8, "__sanitizer_report_sti_atomic_b8", {1, 2, 3}},
    {InstrType::ST_DEV_B64, "__sanitizer_report_st_dev_b64", {1, 2}},
    {InstrType::ST_DEV_B32, "__sanitizer_report_st_dev_b32", {1, 2}},
    {InstrType::ST_DEV_B16, "__sanitizer_report_st_dev_b16", {1, 2}},
    {InstrType::ST_DEV_B8, "__sanitizer_report_st_dev_b8", {1, 2}},
    {InstrType::LD_DEV_B64, "__sanitizer_report_ld_dev_b64", {1, 2}},
    {InstrType::LD_DEV_B32, "__sanitizer_report_ld_dev_b32", {1, 2}},
    {InstrType::LD_DEV_B16, "__sanitizer_report_ld_dev_b16", {1, 2}},
    {InstrType::LD_DEV_B8, "__sanitizer_report_ld_dev_b8", {1, 2}},
    {InstrType::SCALAR_ATOM_U32, "__sanitizer_report_scalar_atom_u32", {1}},
    {InstrType::SCALAR_ATOM_S32, "__sanitizer_report_scalar_atom_s32", {1}},
    {InstrType::SCALAR_ATOM_U64, "__sanitizer_report_scalar_atom_u64", {1}},
    {InstrType::SCALAR_ATOM_S64, "__sanitizer_report_scalar_atom_s64", {1}},
    {InstrType::SCALAR_RED_U32, "__sanitizer_report_scalar_red_u32", {0}},
    {InstrType::SCALAR_RED_S32, "__sanitizer_report_scalar_red_s32", {0}},
    {InstrType::SCALAR_RED_F16, "__sanitizer_report_scalar_red_f16", {0}},
    {InstrType::SCALAR_RED_BF16, "__sanitizer_report_scalar_red_bf16", {0}},
    {InstrType::SCALAR_RED_F32, "__sanitizer_report_scalar_red_f32", {0}},
    /// load_store.cpp end

    /// dc_preload.cpp start
    {InstrType::DC_PRELOAD, "__sanitizer_report_dc_preload", {0, 1}},
    {InstrType::DC_PRELOADI, "__sanitizer_report_dc_preloadi", {0, 1}},
    /// dc_preload.cpp end

    /// simt_load_store.cpp start
    {InstrType::SIMT_LDG_U8, "__sanitizer_report_simt_ldg_u8", {1, 2}},
    {InstrType::SIMT_LDG_U16, "__sanitizer_report_simt_ldg_u16", {1, 2}},
    {InstrType::SIMT_LDG_S8, "__sanitizer_report_simt_ldg_s8", {1, 2}},
    {InstrType::SIMT_LDG_S16, "__sanitizer_report_simt_ldg_s16", {1, 2}},
    {InstrType::SIMT_LDG_B32, "__sanitizer_report_simt_ldg_b32", {1, 2}},
    {InstrType::SIMT_LDG_B64, "__sanitizer_report_simt_ldg_b64", {1, 2}},
    {InstrType::SIMT_LDG_B128, "__sanitizer_report_simt_ldg_b128", {1, 2}},
    {InstrType::SIMT_STG_B8, "__sanitizer_report_simt_stg_b8", {1, 2}},
    {InstrType::SIMT_STG_B16, "__sanitizer_report_simt_stg_b16", {1, 2}},
    {InstrType::SIMT_STG_B32, "__sanitizer_report_simt_stg_b32", {1, 2}},
    {InstrType::SIMT_STG_B64, "__sanitizer_report_simt_stg_b64", {1, 2}},
    {InstrType::SIMT_STG_B128, "__sanitizer_report_simt_stg_b128", {1, 2}},
    {InstrType::SIMT_LDS_U8, "__sanitizer_report_simt_lds_u8", {1, 2}},
    {InstrType::SIMT_LDS_S8, "__sanitizer_report_simt_lds_s8", {1, 2}},
    {InstrType::SIMT_LDS_U16, "__sanitizer_report_simt_lds_u16", {1, 2}},
    {InstrType::SIMT_LDS_S16, "__sanitizer_report_simt_lds_s16", {1, 2}},
    {InstrType::SIMT_LDS_B32, "__sanitizer_report_simt_lds_b32", {1, 2}},
    {InstrType::SIMT_LDS_B64, "__sanitizer_report_simt_lds_b64", {1, 2}},
    {InstrType::SIMT_LDS_B128, "__sanitizer_report_simt_lds_b128", {1, 2}},
    {InstrType::SIMT_STS_B8, "__sanitizer_report_simt_sts_b8", {1, 2}},
    {InstrType::SIMT_STS_B16, "__sanitizer_report_simt_sts_b16", {1, 2}},
    {InstrType::SIMT_STS_B32, "__sanitizer_report_simt_sts_b32", {1, 2}},
    {InstrType::SIMT_STS_B64, "__sanitizer_report_simt_sts_b64", {1, 2}},
    {InstrType::SIMT_STS_B128, "__sanitizer_report_simt_sts_b128", {1, 2}},
    {InstrType::SIMT_LDK_U8, "__sanitizer_report_simt_ldk_u8", {1, 2}},
    {InstrType::SIMT_LDK_S8, "__sanitizer_report_simt_ldk_s8", {1, 2}},
    {InstrType::SIMT_LDK_U16, "__sanitizer_report_simt_ldk_u16", {1, 2}},
    {InstrType::SIMT_LDK_S16, "__sanitizer_report_simt_ldk_s16", {1, 2}},
    {InstrType::SIMT_LDK_B32, "__sanitizer_report_simt_ldk_b32", {1, 2}},
    {InstrType::SIMT_STK_B8, "__sanitizer_report_simt_stk_b8", {1, 2}},
    {InstrType::SIMT_STK_B16, "__sanitizer_report_simt_stk_b16", {1, 2}},
    {InstrType::SIMT_STK_B32, "__sanitizer_report_simt_stk_b32", {1, 2}},
    {InstrType::SIMT_LD_U8, "__sanitizer_report_simt_ld_u8", {1, 2}},
    {InstrType::SIMT_LD_S8, "__sanitizer_report_simt_ld_s8", {1, 2}},
    {InstrType::SIMT_LD_U16, "__sanitizer_report_simt_ld_u16", {1, 2}},
    {InstrType::SIMT_LD_S16, "__sanitizer_report_simt_ld_s16", {1, 2}},
    {InstrType::SIMT_LD_B32, "__sanitizer_report_simt_ld_b32", {1, 2}},
    {InstrType::SIMT_ST_B8, "__sanitizer_report_simt_st_b8", {1, 2}},
    {InstrType::SIMT_ST_B16, "__sanitizer_report_simt_st_b16", {1, 2}},
    {InstrType::SIMT_ST_B32, "__sanitizer_report_simt_st_b32", {1, 2}},
    {InstrType::ATOM_MAX_S_S32, "__sanitizer_report_atom_max_s_s32", {1}},
    {InstrType::ATOM_MIN_S_S32, "__sanitizer_report_atom_min_s_s32", {1}},
    {InstrType::ATOM_ADD_S_S32, "__sanitizer_report_atom_add_s_s32", {1}},
    {InstrType::ATOM_ADD_S_U32, "__sanitizer_report_atom_add_s_u32", {1}},
    {InstrType::ATOM_EXCH_S_U32, "__sanitizer_report_atom_exch_s_u32", {1}},
    {InstrType::ATOM_MIN_S_U32, "__sanitizer_report_atom_min_s_u32", {1}},
    {InstrType::ATOM_CAS_S_U32, "__sanitizer_report_atom_cas_s_u32", {1}},
    {InstrType::ATOM_MAX_S_U32, "__sanitizer_report_atom_max_s_u32", {1}},
    {InstrType::ATOM_EXCH_G_U64, "__sanitizer_report_atom_exch_g_u64", {1}},
    {InstrType::ATOM_MIN_G_U64, "__sanitizer_report_atom_min_g_u64", {1}},
    {InstrType::ATOM_ADD_G_U64, "__sanitizer_report_atom_add_g_u64", {1}},
    {InstrType::ATOM_CAS_G_U64, "__sanitizer_report_atom_cas_g_u64", {1}},
    {InstrType::ATOM_MAX_G_U64, "__sanitizer_report_atom_max_g_u64", {1}},
    {InstrType::ATOM_ADD_G_S64, "__sanitizer_report_atom_add_g_s64", {1}},
    {InstrType::ATOM_MIN_G_S64, "__sanitizer_report_atom_min_g_s64", {1}},
    {InstrType::ATOM_MAX_G_S64, "__sanitizer_report_atom_max_g_s64", {1}},
    {InstrType::ATOM_EXCH_G_U32, "__sanitizer_report_atom_exch_g_u32", {1}},
    {InstrType::ATOM_MIN_G_U32, "__sanitizer_report_atom_min_g_u32", {1}},
    {InstrType::ATOM_CAS_G_U32, "__sanitizer_report_atom_cas_g_u32", {1}},
    {InstrType::ATOM_MAX_G_U32, "__sanitizer_report_atom_max_g_u32", {1}},
    {InstrType::ATOM_ADD_G_U32, "__sanitizer_report_atom_add_g_u32", {1}},
    {InstrType::ATOM_ADD_G_S32, "__sanitizer_report_atom_add_g_s32", {1}},
    {InstrType::ATOM_MIN_G_S32, "__sanitizer_report_atom_min_g_s32", {1}},
    {InstrType::ATOM_MAX_G_S32, "__sanitizer_report_atom_max_g_s32", {1}},
    {InstrType::RED_G_U32, "__sanitizer_report_red_g_u32", {0}},
    {InstrType::RED_G_S32, "__sanitizer_report_red_g_s32", {0}},
    {InstrType::RED_G_FP16, "__sanitizer_report_red_g_fp16", {0}},
    {InstrType::RED_G_FP32, "__sanitizer_report_red_g_fp32", {0}},
    {InstrType::RED_G_F16X2, "__sanitizer_report_red_g_f16x2", {0}},
    {InstrType::RED_G_BF16, "__sanitizer_report_red_g_bf16", {0}},
    {InstrType::RED_G_BF16X2, "__sanitizer_report_red_g_bf16x2", {0}},
    {InstrType::RED_S_U32, "__sanitizer_report_red_s_u32", {0}},
    {InstrType::RED_S_S32, "__sanitizer_report_red_s_s32", {0}},
    {InstrType::RED_S_FP16, "__sanitizer_report_red_s_fp16", {0}},
    {InstrType::RED_S_FP32, "__sanitizer_report_red_s_fp32", {0}},
    {InstrType::RED_S_BF16, "__sanitizer_report_red_s_bf16", {0}},
    /// simt_load_store.cpp end

    /// vector_instructions.cpp start
    {InstrType::MOVEVA, "__sanitizer_report_moveva", {0, 1, 2, 3}},
    {InstrType::LDVA, "__sanitizer_report_ldva", {0, 1, 2}},
    {InstrType::VTRANSPOSE_B16, "__sanitizer_report_vtranspose", {0, 1}},
    {InstrType::SCATTER_VNCHWCONV_B16, "__sanitizer_report_scatter_vnchwconv_b16", {0, 1, 2}},
    {InstrType::SCATTER_VNCHWCONV_B32, "__sanitizer_report_scatter_vnchwconv_b32", {0, 1, 2}},
    {InstrType::SCATTER_VNCHWCONV_B8, "__sanitizer_report_scatter_vnchwconv_b8", {0, 1, 2, 3, 4}},
    {InstrType::VBS32_F16, "__sanitizer_report_vbs32_f16", {0, 1, 2, 3}},
    {InstrType::VBS32_F32, "__sanitizer_report_vbs32_f32", {0, 1, 2, 3}},
    {InstrType::VMRGSORT_F16, "__sanitizer_report_vmrgsort4_f16", {0, 1, 2, 3}},
    {InstrType::VMRGSORT_F32, "__sanitizer_report_vmrgsort4_f32", {0, 1, 2, 3}},
    /// vector_instructions.cpp end

    /// cube_mte_registers.cpp start
    {InstrType::MTE2_SRC_PARA, "__sanitizer_report_set_mte2_src_para", {0}},
    {InstrType::LOOP3_PARA, "__sanitizer_report_set_loop3_para", {0}},
    {InstrType::CHANNEL_PARA, "__sanitizer_report_set_channel_para", {0}},
    {InstrType::SET_FMATRIX, "__sanitizer_report_set_fmatrix", {0}},
    {InstrType::SET_FMATRIX_B, "__sanitizer_report_set_fmatrix_b", {0}},
    {InstrType::SET_FMATRIX_DUAL_0, "__sanitizer_report_set_fmatrix_dual_0", {0}},
    {InstrType::SET_FMATRIX_DUAL_1, "__sanitizer_report_set_fmatrix_dual_1", {0}},
    {InstrType::SET_L3D_RPT, "__sanitizer_report_set_l3d_rpt", {0}},
    {InstrType::SET_L3D_RPT_B, "__sanitizer_report_set_l3d_rpt_b", {0}},
    {InstrType::SET_PADDING, "__sanitizer_report_set_padding", {0}},
    {InstrType::SET_PADDING_B, "__sanitizer_report_set_padding_b", {0}},
    /// cube_mte_registers.cpp end

    /// cube_instructions.cpp start
    {InstrType::MAD_S8, "__sanitizer_report_mad_s8", {0, 1, 2, 3}},
    {InstrType::MAD_F16_F32, "__sanitizer_report_mad_f16_f32", {0, 1, 2, 3}},
    {InstrType::MAD_BF16_F32, "__sanitizer_report_mad_bf16_f32", {0, 1, 2, 3}},
    {InstrType::MAD_F32_F32, "__sanitizer_report_mad_f32_f32", {0, 1, 2, 3}},
    {InstrType::MAD_E4M3_E4M3, "__sanitizer_report_mad_e4m3_e4m3", {0, 1, 2, 3}},
    {InstrType::MAD_E4M3_E5M2, "__sanitizer_report_mad_e4m3_e5m2", {0, 1, 2, 3}},
    {InstrType::MAD_E5M2_E4M3, "__sanitizer_report_mad_e5m2_e4m3", {0, 1, 2, 3}},
    {InstrType::MAD_E5M2_E5M2, "__sanitizer_report_mad_e5m2_e5m2", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E1M2X2_E1M2X2, "__sanitizer_report_mad_mx_e1m2_e1m2", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E1M2X2_E2M1X2, "__sanitizer_report_mad_mx_e1m2_e2m1", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E2M1X2_E1M2X2, "__sanitizer_report_mad_mx_e2m1_e1m2", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E2M1X2_E2M1X2, "__sanitizer_report_mad_mx_e2m1_e2m1", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E4M3_E4M3, "__sanitizer_report_mad_mx_e4m3_e4m3", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E4M3_E5M2, "__sanitizer_report_mad_mx_e4m3_e5m2", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E5M2_E4M3, "__sanitizer_report_mad_mx_e5m2_e4m3", {0, 1, 2, 3}},
    {InstrType::MAD_MX_E5M2_E5M2, "__sanitizer_report_mad_mx_e5m2_e5m2", {0, 1, 2, 3}},
    /// cube_instructions.cpp end

    /// sync_instructions.cpp start
    {InstrType::SET_FLAG, "__sanitizer_report_set_flag", {0, 1, 2}},
    {InstrType::SET_FLAGI, "__sanitizer_report_set_flagi", {0, 1, 2}},
    {InstrType::WAIT_FLAG, "__sanitizer_report_wait_flag", {0, 1, 2}},
    {InstrType::WAIT_FLAGI, "__sanitizer_report_wait_flagi", {0, 1, 2}},
    {InstrType::HSET_FLAG, "__sanitizer_report_hset_flag", {0, 1, 2, 3, 4}},
    {InstrType::HWAIT_FLAG, "__sanitizer_report_hwait_flag", {0, 1, 2, 3, 4}},
    {InstrType::HSET_FLAGI, "__sanitizer_report_hset_flagi", {0, 1, 2, 3, 4}},
    {InstrType::HWAIT_FLAGI, "__sanitizer_report_hwait_flagi", {0, 1, 2, 3, 4}},

    {InstrType::GET_BUF, "__sanitizer_report_get_buf", {0, 1, 2}},
    {InstrType::GET_BUFI, "__sanitizer_report_get_bufi", {0, 1, 2}},
    {InstrType::RLS_BUF, "__sanitizer_report_rls_buf", {0, 1, 2}},
    {InstrType::RLS_BUFI, "__sanitizer_report_rls_bufi", {0, 1, 2}},
    {InstrType::GET_BUF_V, "__sanitizer_report_get_buf_v", {0, 1, 2}},
    {InstrType::GET_BUFI_V, "__sanitizer_report_get_bufi_v", {0, 1, 2}},
    {InstrType::RLS_BUF_V, "__sanitizer_report_rls_buf_v", {0, 1, 2}},
    {InstrType::RLS_BUFI_V, "__sanitizer_report_rls_bufi_v", {0, 1, 2}},
    {InstrType::WAIT_FLAG_DEV, "__sanitizer_report_wait_flag_dev_pipe", {0, 1}},
    {InstrType::WAIT_FLAG_DEVI, "__sanitizer_report_wait_flag_devi_pipe", {0, 1}},
    {InstrType::SET_CROSS_CORE, "__sanitizer_report_set_cross_core", {0, 1}},
    {InstrType::WAIT_INTRA_BLOCK, "__sanitizer_report_wait_intra_block", {0, 1}},
    {InstrType::SET_INTRA_BLOCK, "__sanitizer_report_set_intra_block", {0, 1}},
    {InstrType::WAIT_INTRA_BLOCKI, "__sanitizer_report_wait_intra_blocki", {0, 1}},
    {InstrType::SET_INTRA_BLOCKI, "__sanitizer_report_set_intra_blocki", {0, 1}},
    {InstrType::SET_FLAG_V, "__sanitizer_report_set_flag_v", {0, 1}},
    {InstrType::SET_FLAGI_V, "__sanitizer_report_set_flagi_v", {0, 1}},
    {InstrType::WAIT_FLAG_V, "__sanitizer_report_wait_flag_v", {0, 1}},
    {InstrType::WAIT_FLAGI_V, "__sanitizer_report_wait_flagi_v", {0, 1}},
    {InstrType::SET_CROSS_CORE_V, "__sanitizer_report_set_cross_core_v", {0}},
    {InstrType::WAIT_FLAG_DEV_V, "__sanitizer_report_wait_flag_dev_pipe_v", {0}},
    {InstrType::WAIT_FLAG_DEVI_V, "__sanitizer_report_wait_flag_devi_pipe_v", {0}},
    {InstrType::SET_INTRA_BLOCK_V, "__sanitizer_report_set_intra_block_v", {0}},
    {InstrType::SET_INTRA_BLOCKI_V, "__sanitizer_report_set_intra_blocki_v", {0}},
    {InstrType::WAIT_INTRA_BLOCK_V, "__sanitizer_report_wait_intra_block_v", {0}},
    {InstrType::WAIT_INTRA_BLOCKI_V, "__sanitizer_report_wait_intra_blocki_v", {0}},
    {InstrType::PIPE_BARRIER, "__sanitizer_report_pipe_barrier", {0}},
    /// sync_instructions.cpp end

    /// registers_instructions.cpp start
    {InstrType::MOVE_MASK_0, "__sanitizer_report_set_vector_mask_0", {0, 1}},
    {InstrType::MOVE_MASK_1, "__sanitizer_report_set_vector_mask_1", {0, 1}},
    {InstrType::MOVE_MASK_IMM, "__sanitizer_report_set_vector_mask_imm", {0, 1}},
    {InstrType::SET_CTRL, "__sanitizer_report_set_ctrl", {0}},
    {InstrType::SET_FFTS_BASE_ADDR, "__sanitizer_report_set_ffts_base_addr", {0}},
    {InstrType::SET_FPC, "__sanitizer_report_set_fpc", {0}},
    {InstrType::SET_QUANT_PRE, "__sanitizer_report_set_quant_pre", {0}},
    {InstrType::SET_QUANT_POST, "__sanitizer_report_set_quant_post", {0}},
    {InstrType::SET_LRELU_ALPHA, "__sanitizer_report_set_lrelu_alpha", {0}},
    /// registers_instructions.cpp end

    /// simt_sync_instructions.cpp start
    {InstrType::THREAD_BLOCK_BARRIER, "__sanitizer_report_thread_block_barrier", {}},
    /// simt_sync_instructions.cpp end

    // simt_start_end_instructions.cpp start
    {InstrType::SIMT_END, "__sanitizer_report_simt_end", {}},
    // simt_start_end_instructions.cpp end

    // main_scalar_empty_instructions.cpp start
    {InstrType::SIMT_CALL, "__sanitizer_report_simt_call", {}},
    // main_scalar_empty_instructions.cpp end

    {InstrType::MSTX_STUB_CALLI, "__sanitizer_report___mstx_dfx_report_stub_calli", {0, 1, 2}},
    {InstrType::MSTX_STUB_CALL, "__sanitizer_report___mstx_dfx_report_stub_call", {0, 1, 2}},
};

void MSBitAtInit()
{
    for (auto &stub : bindStubs) {
        Bind(stub.instrType, stub.injectedFuncName, stub.paraMask);
    }
}
}
