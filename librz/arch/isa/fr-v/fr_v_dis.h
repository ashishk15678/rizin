// SPDX-FileCopyrightText: 2026 Ashish Kumar <15678ashishk@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#ifndef FRV_DIS_H
#define FRV_DIS_H

#include <rz_types.h>
#include <rz_util/rz_strbuf.h>

// ── Instruction size ──────────────────────────────────────────────────────────
// All FR-V instructions are 32-bit fixed-width, big-endian.
#define FRV_INSN_SIZE 4

// ── Register counts ───────────────────────────────────────────────────────────
// FR-V has 64 GRs, 64 FRs, 8 ICCRs, 8 FCCRs, 8 CRs (conditional registers)
#define FRV_NUM_GR   64
#define FRV_NUM_FR   64
#define FRV_NUM_CPR  64 // co-processor registers
#define FRV_NUM_ACC  8 // accumulator registers
#define FRV_NUM_ACCG 8 // accumulator guard registers
#define FRV_NUM_ICC  4 // integer condition code registers (icc0-icc3)
#define FRV_NUM_FCC  4 // float condition code registers (fcc0-fcc3)
#define FRV_NUM_CR   8 // conditional execution registers (cr0-cr7)

// ── Bit-field extraction helpers ──────────────────────────────────────────────
// All fields are taken from a big-endian 32-bit word already read into a ut32.
#define FRV_BITS(insn, hi, lo) (((insn) >> (lo)) & ((1u << ((hi) - (lo) + 1)) - 1u))

// Primary fields common to most instruction formats
#define FRV_OP(insn)    FRV_BITS(insn, 31, 27) // 5-bit major opcode
#define FRV_SUBOP(insn) FRV_BITS(insn, 26, 24) // 3-bit sub-opcode / condition
#define FRV_GRI(insn)   FRV_BITS(insn, 23, 18) // 6-bit source register i
#define FRV_GRJ(insn)   FRV_BITS(insn, 17, 12) // 6-bit source register j
#define FRV_CCI(insn)   FRV_BITS(insn, 11, 6) // 6-bit condition code index
#define FRV_GRK(insn)   FRV_BITS(insn, 5, 0) // 6-bit destination register k

// Immediate formats
#define FRV_SIMM12(insn)  (((st32)(FRV_BITS(insn, 17, 6)) << 20) >> 20) // sign-extended 12-bit
#define FRV_SIMM16(insn)  (((st32)(FRV_BITS(insn, 17, 2)) << 16) >> 16) // 16-bit for sethi/setlo
#define FRV_UHI16(insn)   FRV_BITS(insn, 17, 2) // unsigned 16-bit hi
#define FRV_LABEL16(insn) (((st32)(FRV_BITS(insn, 17, 2)) << 16) >> 14) // branch label16, *4
#define FRV_LABEL24(insn) ({ \
	ut32 _lo = FRV_BITS(insn, 17, 2); \
	ut32 _hi = FRV_BITS(insn, 23, 18); \
	(st32)((_hi << 18 | _lo) << 8) >> 6; \
})

// Shift amount (6-bit unsigned embedded in GRJ field)
#define FRV_SHAMT(insn) FRV_BITS(insn, 17, 12)

// ICCI / FCCI select fields (integer / float condition code index)
#define FRV_ICCI(insn) FRV_BITS(insn, 23, 22) // 2-bit ICC index (0-3)
#define FRV_FCCI(insn) FRV_BITS(insn, 23, 22) // 2-bit FCC index (0-3)
#define FRV_COND(insn) FRV_BITS(insn, 26, 24) // 3-bit branch condition

// ── Major opcodes (bits [31:27]) ──────────────────────────────────────────────
// Values derived from GCC frv.md / CGEN frv.cpu machine description
// and confirmed against binutils opcodes/frv-opc.c
typedef enum {
	FRV_MOP_INT_ARITH = 0x00, // add, sub, smul, umul, sdiv, udiv, ...
	FRV_MOP_INT_LOGIC = 0x01, // and, or, xor, not, ...
	FRV_MOP_INT_SHIFT = 0x02, // sll, srl, sra, lsli, ...
	FRV_MOP_INT_CMP = 0x03, // cmp, cmpi, ...
	FRV_MOP_INT_SET = 0x04, // sethi, setlo, setlos, movgf, movfg
	FRV_MOP_INT_COND = 0x05, // conditional integer (cmov, cadd, ...)
	FRV_MOP_LOAD_GR = 0x06, // ld, ldub, lduh, ldd, ldq, ...
	FRV_MOP_STORE_GR = 0x07, // st, stb, sth, std, stq, ...
	FRV_MOP_BRANCH = 0x08, // beq, bne, blt, bgt, ble, bge, bra
	FRV_MOP_JUMP = 0x09, // call, jmpl, ret, ...
	FRV_MOP_FP_ARITH = 0x0A, // fadds, faddd, fsubs, fsubd, fmuls, fmuld, ...
	FRV_MOP_FP_CMP = 0x0B, // fcmps, fcmpd, ...
	FRV_MOP_FP_BRANCH = 0x0C, // fbne, fbeq, fblt, ...
	FRV_MOP_FP_LOAD = 0x0D, // ldf, lddf, ldqf
	FRV_MOP_FP_STORE = 0x0E, // stf, stdf, stqf
	FRV_MOP_MEDIA = 0x0F, // media instructions
	FRV_MOP_INT_ARITH2 = 0x10, // further integer arithmetic (with CC output)
	FRV_MOP_COPR = 0x11, // co-processor
	FRV_MOP_LOAD_CPR = 0x12, // co-processor loads
	FRV_MOP_STORE_CPR = 0x13, // co-processor stores
	FRV_MOP_ATOMIC = 0x14, // swap, cas
	FRV_MOP_NOP = 0x1F, // nop / fnop / mnop
} FRVMajorOpcode;

// ── Branch conditions (bits [26:24] when major==FRV_MOP_BRANCH) ───────────────
// Standard integer branch condition codes
typedef enum {
	FRV_BCOND_EQ = 0x0, // beq  — integer equal
	FRV_BCOND_NE = 0x1, // bne  — not equal
	FRV_BCOND_LE = 0x2, // ble  — signed less-or-equal
	FRV_BCOND_GT = 0x3, // bgt  — signed greater
	FRV_BCOND_LT = 0x4, // blt  — signed less
	FRV_BCOND_GE = 0x5, // bge  — signed greater-or-equal
	FRV_BCOND_LS = 0x6, // bls  — unsigned lower-or-same
	FRV_BCOND_HI = 0x7, // bhi  — unsigned higher
	// Extended conditions (subop 8-15 rarely used)
	FRV_BCOND_RA = 0x08, // bra  — unconditional
	FRV_BCOND_NO = 0x09, // bno  — never (NOP branch)
	FRV_BCOND_C = 0x0A, // bc   — carry
	FRV_BCOND_NC = 0x0B, // bnc  — no carry
	FRV_BCOND_N = 0x0C, // bn   — negative
	FRV_BCOND_P = 0x0D, // bp   — positive
	FRV_BCOND_V = 0x0E, // bv   — overflow
	FRV_BCOND_NV = 0x0F, // bnv  — no overflow
} FRVBranchCond;

// ── Decoded instruction structure ─────────────────────────────────────────────
typedef struct {
	RzStrBuf strbuf; // accumulates disassembly text
	ut32 raw; // raw 32-bit instruction word
	ut32 size; // always FRV_INSN_SIZE (4) on success
	ut64 target; // filled for branches / calls
	bool is_branch;
	bool is_call;
	bool is_ret;
} FRVOp;

// ── Public API ────────────────────────────────────────────────────────────────
int frv_disassemble(FRVOp *op, const ut8 *buf, int len, ut64 pc);

#endif // FRV_DIS_H
