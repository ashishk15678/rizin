// SPDX-FileCopyrightText: 2024 contributors
// SPDX-License-Identifier: LGPL-3.0-only
//
// Fujitsu FR-V RzAnalysis plugin — analysis_frv.c
//
// Provides Rizin's analysis layer with instruction type classification
// (branch, call, return, load, store, etc.) derived from the disassembler.
//
// References:
//   • GCC gcc/config/frv/frv.c enum frv_insn_group {GROUP_I, GROUP_FM, GROUP_B, GROUP_C}
//   • Rizin librz/analysis/arch/hexagon architecture as structural template

#include <rz_analysis.h>
#include <rz_lib.h>
#include "../isa/fr-v/fr_v_dis.h"

// ── Type classifier ───────────────────────────────────────────────────────────
//
// Reads one instruction and fills the RzAnalysisOp fields.

static int frv_op(RzAnalysis *a, RzAnalysisOp *aop,
                  ut64 addr, const ut8 *buf, int len,
                  RzAnalysisOpMask mask)
{
    if (!buf || len < FRV_INSN_SIZE) {
        return -1;
    }

    aop->size   = FRV_INSN_SIZE;
    aop->addr   = addr;
    aop->type   = RZ_ANALYSIS_OP_TYPE_UNK;
    aop->jump   = UT64_MAX;
    aop->fail   = UT64_MAX;
    aop->val    = UT64_MAX;

    FRVOp op = { 0 };
    if (frv_disassemble(&op, buf, len, addr) != 0) {
        rz_strbuf_fini(&op.strbuf);
        return -1;
    }

    ut32 insn  = op.raw;
    ut32 major = FRV_OP(insn);

    switch (major) {
    // ── Integer arithmetic / logical / shift ──────────────────────────────
    case FRV_MOP_INT_ARITH:
    case FRV_MOP_INT_ARITH2:
        aop->type = RZ_ANALYSIS_OP_TYPE_ADD; // simplified; sub/mul need refine
        if (FRV_SUBOP(insn) == 1) aop->type = RZ_ANALYSIS_OP_TYPE_SUB;
        if (FRV_SUBOP(insn) == 2 || FRV_SUBOP(insn) == 3)
            aop->type = RZ_ANALYSIS_OP_TYPE_MUL;
        break;

    case FRV_MOP_INT_LOGIC:
        switch (FRV_SUBOP(insn)) {
        case 0: aop->type = RZ_ANALYSIS_OP_TYPE_AND; break;
        case 1: aop->type = RZ_ANALYSIS_OP_TYPE_OR;  break;
        case 2: aop->type = RZ_ANALYSIS_OP_TYPE_XOR; break;
        default: aop->type = RZ_ANALYSIS_OP_TYPE_UNK; break;
        }
        break;

    case FRV_MOP_INT_SHIFT:
        switch (FRV_SUBOP(insn)) {
        case 0: case 3: aop->type = RZ_ANALYSIS_OP_TYPE_SHL; break;
        case 1: case 4: aop->type = RZ_ANALYSIS_OP_TYPE_SHR; break;
        case 2: case 5: aop->type = RZ_ANALYSIS_OP_TYPE_SAR; break;
        default: aop->type = RZ_ANALYSIS_OP_TYPE_UNK; break;
        }
        break;

    case FRV_MOP_INT_CMP:
    case FRV_MOP_FP_CMP:
        aop->type = RZ_ANALYSIS_OP_TYPE_CMP;
        break;

    case FRV_MOP_INT_SET:
        aop->type = RZ_ANALYSIS_OP_TYPE_MOV;
        break;

    case FRV_MOP_INT_COND:
        aop->type = RZ_ANALYSIS_OP_TYPE_CMOV;
        break;

    // ── Load / Store ──────────────────────────────────────────────────────
    case FRV_MOP_LOAD_GR:
    case FRV_MOP_FP_LOAD:
        aop->type = RZ_ANALYSIS_OP_TYPE_LOAD;
        break;

    case FRV_MOP_STORE_GR:
    case FRV_MOP_FP_STORE:
        aop->type = RZ_ANALYSIS_OP_TYPE_STORE;
        break;

    // ── Control flow ──────────────────────────────────────────────────────
    case FRV_MOP_BRANCH:
    case FRV_MOP_FP_BRANCH:
        if (op.is_branch) {
            aop->type = RZ_ANALYSIS_OP_TYPE_CJMP;
            aop->jump = op.target;
            aop->fail = addr + FRV_INSN_SIZE;
            // bra (unconditional) → JMP, not CJMP
            if (FRV_COND(insn) == FRV_BCOND_RA ||
                FRV_COND(insn) == 0x07 /* fbra */) {
                aop->type = RZ_ANALYSIS_OP_TYPE_JMP;
                aop->fail = UT64_MAX;
            }
        }
        break;

    case FRV_MOP_JUMP:
        if (op.is_ret) {
            aop->type = RZ_ANALYSIS_OP_TYPE_RET;
        } else if (op.is_call) {
            aop->type = RZ_ANALYSIS_OP_TYPE_CALL;
            aop->jump = op.target;
        } else if (op.is_branch) {
            aop->type = RZ_ANALYSIS_OP_TYPE_JMP;
            aop->jump = op.target;
        }
        break;

    // ── Atomic ────────────────────────────────────────────────────────────
    case FRV_MOP_ATOMIC:
        aop->type = RZ_ANALYSIS_OP_TYPE_XCHG;
        break;

    // ── NOP ───────────────────────────────────────────────────────────────
    case FRV_MOP_NOP:
        aop->type = RZ_ANALYSIS_OP_TYPE_NOP;
        break;

    // ── Float arithmetic / media ──────────────────────────────────────────
    case FRV_MOP_FP_ARITH:
    case FRV_MOP_MEDIA:
        aop->type = RZ_ANALYSIS_OP_TYPE_ADD; // approximation
        break;

    default:
        aop->type = RZ_ANALYSIS_OP_TYPE_UNK;
        break;
    }

    // Copy disassembly text if the mask requests it
    if (mask & RZ_ANALYSIS_OP_MASK_DISASM) {
        aop->mnemonic = strdup(rz_strbuf_get(&op.strbuf));
    }

    rz_strbuf_fini(&op.strbuf);
    return FRV_INSN_SIZE;
}

// ── Analysis plugin descriptor ────────────────────────────────────────────────

RzAnalysisPlugin rz_analysis_plugin_fr_v = {
    .name      = "fr_v",
    .arch      = "fr_v",
    .bits      = 32,
    // .endian    = RZ_SYS_ENDIAN_BIG,
    .desc      = "Fujitsu FR-V RISC-VLIW analysis plugin",
    .license   = "LGPL-3",
    .op        = frv_op,
};

#ifndef CORELIB
RZ_API RzLibStruct rizin_plugin = {
    .type    = RZ_LIB_TYPE_ANALYSIS,
    .data    = &rz_analysis_plugin_fr_v,
    .version = RZ_VERSION,
};
#endif
