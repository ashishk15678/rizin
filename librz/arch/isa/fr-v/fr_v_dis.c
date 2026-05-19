// SPDX-FileCopyrightText: 2026 Ashish Kumar <15678ashishk@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#include "fr_v_dis.h"
#include <rz_endian.h>
#include <string.h>

// clang-format off
static const char *const frv_gr_names[FRV_NUM_GR] = {
    "gr0",  "sp",   "fp",   "gr3",  "gr4",  "gr5",  "gr6",  "gr7",
    "gr8",  "gr9",  "gr10", "gr11", "gr12", "gr13", "gr14", "gr15",
    "gr16", "gr17", "gr18", "gr19", "gr20", "gr21", "gr22", "gr23",
    "gr24", "gr25", "gr26", "gr27", "gr28", "gr29", "gr30", "gr31",
    "gr32", "gr33", "gr34", "gr35", "gr36", "gr37", "gr38", "gr39",
    "gr40", "gr41", "gr42", "gr43", "gr44", "gr45", "gr46", "gr47",
    "gr48", "gr49", "gr50", "gr51", "gr52", "gr53", "gr54", "gr55",
    "gr56", "gr57", "gr58", "gr59", "gr60", "gr61", "gr62", "gr63",
};

static const char *const frv_fr_names[FRV_NUM_FR] = {
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
    "fr16", "fr17", "fr18", "fr19", "fr20", "fr21", "fr22", "fr23",
    "fr24", "fr25", "fr26", "fr27", "fr28", "fr29", "fr30", "fr31",
    "fr32", "fr33", "fr34", "fr35", "fr36", "fr37", "fr38", "fr39",
    "fr40", "fr41", "fr42", "fr43", "fr44", "fr45", "fr46", "fr47",
    "fr48", "fr49", "fr50", "fr51", "fr52", "fr53", "fr54", "fr55",
    "fr56", "fr57", "fr58", "fr59", "fr60", "fr61", "fr62", "fr63",
};

static const char *const frv_icc_names[FRV_NUM_ICC] = {
    "icc0", "icc1", "icc2", "icc3",
};

static const char *const frv_fcc_names[FRV_NUM_FCC] = {
    "fcc0", "fcc1", "fcc2", "fcc3",
};

static const char *const frv_cr_names[FRV_NUM_CR] = {
    "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
};
// clang-format on

static inline const char *gr(ut32 r) {
	return (r < FRV_NUM_GR) ? frv_gr_names[r] : "?gr";
}
static inline const char *fr(ut32 r) {
	return (r < FRV_NUM_FR) ? frv_fr_names[r] : "?fr";
}
static inline const char *icc(ut32 r) {
	return (r < FRV_NUM_ICC) ? frv_icc_names[r] : "?icc";
}
static inline const char *fcc(ut32 r) {
	return (r < FRV_NUM_FCC) ? frv_fcc_names[r] : "?fcc";
}
static inline const char *cr(ut32 r) {
	return (r < FRV_NUM_CR) ? frv_cr_names[r] : "?cr";
}

// ── Sub-opcode decode tables ──────────────────────────────────────────────────

// Integer arithmetic sub-opcodes (major 0x00)
// Source: frv-opc.c CGEN table, cross-referenced with GCC frv.md patterns
static const char *const frv_arith_mnem[8] = {
	[0] = "add", // add GRi, GRj, GRk
	[1] = "sub", // sub GRi, GRj, GRk
	[2] = "smul", // signed multiply  → double-width result in GRdouble
	[3] = "umul", // unsigned multiply
	[4] = "sdiv", // signed divide
	[5] = "udiv", // unsigned divide
	[6] = "addcc", // add, sets ICC
	[7] = "subcc", // sub, sets ICC
};

static const char *const frv_arith_imm_mnem[8] = {
	[0] = "addi",
	[1] = "subi",
	[2] = "smuli",
	[3] = "umuli",
	[4] = "sdivi",
	[5] = "udivi",
	[6] = "addxi", // add with carry immediate
	[7] = "subxi",
};

static const char *const frv_logic_mnem[8] = {
	[0] = "and",
	[1] = "or",
	[2] = "xor",
	[3] = "nand",
	[4] = "andi",
	[5] = "ori",
	[6] = "xori",
	[7] = "not", // not GRj, GRk  (unary)
};

// Shift sub-opcodes (major 0x02)
static const char *const frv_shift_mnem[8] = {
	[0] = "sll", // logical left shift, reg
	[1] = "srl", // logical right shift, reg
	[2] = "sra", // arithmetic right shift, reg
	[3] = "slli", // immediate
	[4] = "srli",
	[5] = "srai",
	[6] = "lsli", // logical shift left (alias for slli in some docs)
	[7] = "scan", // count leading zeros / scan
};

// Branch condition strings (bits [26:24] when major == FRV_MOP_BRANCH)
// From GCC frv.md branch pattern conditions
static const char *const frv_bcond_str[16] = {
	[FRV_BCOND_EQ] = "beq",
	[FRV_BCOND_NE] = "bne",
	[FRV_BCOND_LE] = "ble",
	[FRV_BCOND_GT] = "bgt",
	[FRV_BCOND_LT] = "blt",
	[FRV_BCOND_GE] = "bge",
	[FRV_BCOND_LS] = "bls",
	[FRV_BCOND_HI] = "bhi",
	[FRV_BCOND_RA] = "bra",
	[FRV_BCOND_NO] = "bno",
	[FRV_BCOND_C] = "bc",
	[FRV_BCOND_NC] = "bnc",
	[FRV_BCOND_N] = "bn",
	[FRV_BCOND_P] = "bp",
	[FRV_BCOND_V] = "bv",
	[FRV_BCOND_NV] = "bnv",
};

// Float branch condition strings (major 0x0C)
static const char *const frv_fbcond_str[8] = {
	[0] = "fbeq",
	[1] = "fbne",
	[2] = "fblt",
	[3] = "fbge",
	[4] = "fble",
	[5] = "fbgt",
	[6] = "fbu", // unordered
	[7] = "fbra", // unconditional float branch
};

// Load/store width strings for GR loads (major 0x06/0x07)
// Sub-opcode [26:24] selects width and sign
static const struct {
	const char *ld;
	const char *st;
	bool indexed; // bit 23 set => GRi+GRj addressing
} frv_ls_ops[8] = {
	[0] = { "ld", "st", false }, // 32-bit word
	[1] = { "ldub", "stb", false }, // 8-bit unsigned
	[2] = { "lduh", "sth", false }, // 16-bit unsigned
	[3] = { "ldd", "std", false }, // 64-bit double
	[4] = { "ldsb", "stb", false }, // 8-bit signed
	[5] = { "ldsh", "sth", false }, // 16-bit signed
	[6] = { "ldq", "stq", false }, // 128-bit quad
	[7] = { "ldbf", "stbf", false }, // byte-swap
};

// Float load/store sub-opcode strings (major 0x0D/0x0E)
static const struct {
	const char *ld;
	const char *st;
} frv_fls_ops[8] = {
	[0] = { "ldf", "stf" }, // 32-bit float
	[1] = { "lddf", "stdf" }, // 64-bit double
	[2] = { "ldqf", "stqf" }, // 128-bit quad float
	[3] = { "ldfu", "stfu" }, // unscaled (no post-incr)
	[4] = { "lddfu", "stdfu" },
	[5] = { NULL, NULL }, // reserved
	[6] = { NULL, NULL },
	[7] = { NULL, NULL },
};

// FP arithmetic sub-opcodes (major 0x0A)
static const char *const frv_fp_arith_mnem[8] = {
	[0] = "fadds",
	[1] = "faddd",
	[2] = "fsubs",
	[3] = "fsubd",
	[4] = "fmuls",
	[5] = "fmuld",
	[6] = "fdivs",
	[7] = "fdivd",
};

// FP compare sub-opcodes (major 0x0B)
static const char *const frv_fp_cmp_mnem[4] = {
	[0] = "fcmps",
	[1] = "fcmpd",
	[2] = "fcmpes", // compare with exception on NaN
	[3] = "fcmped",
};

// Integer arithmetic group (major 0x00)
//
// Two sub-encodings exist:
//   • Register form:   op[31:27] subop[26:24] GRi[23:18] GRj[17:12] CCi[11:6] GRk[5:0]
//   • Immediate form:  op[31:27] subop[26:24] GRi[23:18] simm12[17:6]          GRk[5:0]
//
// The immediate form is selected when bit 11 of the instruction word is 1 in
// some sub-opcodes, OR by a distinct sub-opcode range. Per binutils frv-opc.c,
// the immediate variants use subop values 4-7 for the corresponding reg ops 0-3.
// We split on bit [11]: 0 → register form, 1 → immediate form (fits simm12).

static void decode_int_arith(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	st32 simm = FRV_SIMM12(insn);
	ut32 cci = FRV_CCI(insn);

	bool is_imm = (insn >> 12) & 1; // bit 12 set → immediate form (per CGEN encoding)

	if (!is_imm) {
		// Register form: mnem GRi, GRj, GRk
		const char *mn = (subop < 8) ? frv_arith_mnem[subop] : "??arith";
		if (subop == 6 || subop == 7) {
			// addcc/subcc also write an ICC register
			rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s, %s",
				mn, gr(gri), gr(grj), gr(grk), icc(cci & 0x3));
		} else {
			rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s",
				mn, gr(gri), gr(grj), gr(grk));
		}
	} else {
		// Immediate form: mnem GRi, simm12, GRk
		const char *mn = (subop < 8) ? frv_arith_imm_mnem[subop] : "??arithi";
		rz_strbuf_appendf(&op->strbuf, "%s %s, %d, %s",
			mn, gr(gri), simm, gr(grk));
	}
}

static void decode_int_logic(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	st32 simm = FRV_SIMM12(insn);

	const char *mn = (subop < 8) ? frv_logic_mnem[subop] : "??logic";

	switch (subop) {
	case 0:
	case 1:
	case 2:
	case 3: // reg forms
		rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s", mn, gr(gri), gr(grj), gr(grk));
		break;
	case 4:
	case 5:
	case 6: // immediate forms
		rz_strbuf_appendf(&op->strbuf, "%s %s, %d, %s", mn, gr(gri), simm, gr(grk));
		break;
	case 7: // not GRj, GRk  (unary)
		rz_strbuf_appendf(&op->strbuf, "not %s, %s", gr(grj), gr(grk));
		break;
	default:
		rz_strbuf_appendf(&op->strbuf, "??logic.%u", subop);
		break;
	}
}

static void decode_int_shift(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn); // register shift amount for reg forms
	ut32 grk = FRV_GRK(insn);
	ut32 shamt = FRV_SHAMT(insn) & 0x3F; // 6-bit shift amount for imm forms

	const char *mn = (subop < 8) ? frv_shift_mnem[subop] : "??shift";

	switch (subop) {
	case 0:
	case 1:
	case 2: // register shift amount
		rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s", mn, gr(gri), gr(grj), gr(grk));
		break;
	case 3:
	case 4:
	case 5:
	case 6: // immediate shift amount
		rz_strbuf_appendf(&op->strbuf, "%s %s, #%u, %s", mn, gr(gri), shamt, gr(grk));
		break;
	case 7: // scan GRi, GRj, GRk
		rz_strbuf_appendf(&op->strbuf, "scan %s, %s, %s", gr(gri), gr(grj), gr(grk));
		break;
	default:
		rz_strbuf_appendf(&op->strbuf, "??shift.%u", subop);
		break;
	}
}

static void decode_int_cmp(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 cci = FRV_ICCI(insn); // ICC target [23:22]
	st32 simm = FRV_SIMM12(insn);

	switch (subop) {
	case 0: // cmp GRi, GRj, ICCi
		rz_strbuf_appendf(&op->strbuf, "cmp %s, %s, %s", gr(gri), gr(grj), icc(cci));
		break;
	case 1: // cmpi GRi, simm12, ICCi
		rz_strbuf_appendf(&op->strbuf, "cmpi %s, %d, %s", gr(gri), simm, icc(cci));
		break;
	case 2: // andcc GRi, GRj, GRk, ICCi
		rz_strbuf_appendf(&op->strbuf, "andcc %s, %s, %s, %s",
			gr(gri), gr(grj), gr(FRV_GRK(insn)), icc(cci));
		break;
	case 3: // orcc GRi, GRj, GRk, ICCi
		rz_strbuf_appendf(&op->strbuf, "orcc %s, %s, %s, %s",
			gr(gri), gr(grj), gr(FRV_GRK(insn)), icc(cci));
		break;
	default:
		rz_strbuf_appendf(&op->strbuf, "??cmp.%u", subop);
		break;
	}
}

static void decode_int_set(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 grk = FRV_GRK(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 uhi16 = FRV_UHI16(insn); // upper 16-bit value
	st32 simm16 = (st32)((ut32)uhi16 << 16) >> 16; // sign-extended

	switch (subop) {
	case 0: rz_strbuf_appendf(&op->strbuf, "sethi  #0x%04X, %s", uhi16, gr(grk)); break;
	case 1: rz_strbuf_appendf(&op->strbuf, "setlo  #0x%04X, %s", uhi16, gr(grk)); break;
	case 2: rz_strbuf_appendf(&op->strbuf, "setlos #%d, %s", simm16, gr(grk)); break;
	case 3: rz_strbuf_appendf(&op->strbuf, "movgf  %s, %s", gr(grj), fr(grk)); break;
	case 4: rz_strbuf_appendf(&op->strbuf, "movfg  %s, %s", fr(grj), gr(grk)); break;
	case 5: rz_strbuf_appendf(&op->strbuf, "movgfd %s, %s", gr(grj), fr(grk)); break;
	case 6: rz_strbuf_appendf(&op->strbuf, "movfgd %s, %s", fr(grj), gr(grk)); break;
	case 7: rz_strbuf_appendf(&op->strbuf, "movgs  %s, %s", gr(grj), gr(grk)); break;
	}
}

// Conditional integer group (major 0x05)
// Conditional-execution instructions: cmov, cadd, csub, cor, cand, ...

static void decode_int_cond(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	ut32 cci = FRV_CCI(insn); // encodes CR register + sense

	// CR register is in bits [11:9], sense in bit [8] (per CGEN frv.cpu)
	ut32 crn = (cci >> 3) & 0x7;
	ut32 sense = (cci >> 2) & 0x1; // 0 = true, 1 = false ("n" suffix)
	const char *suf = sense ? "n" : "";

	switch (subop) {
	case 0: rz_strbuf_appendf(&op->strbuf, "cmov%s  %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 1: rz_strbuf_appendf(&op->strbuf, "cadd%s  %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 2: rz_strbuf_appendf(&op->strbuf, "csub%s  %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 3: rz_strbuf_appendf(&op->strbuf, "cand%s  %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 4: rz_strbuf_appendf(&op->strbuf, "cor%s   %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 5: rz_strbuf_appendf(&op->strbuf, "cxor%s  %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	case 6: rz_strbuf_appendf(&op->strbuf, "cnot%s  %s, %s, %s", suf, gr(grj), gr(grk), cr(crn)); break;
	case 7: rz_strbuf_appendf(&op->strbuf, "csmul%s %s, %s, %s, %s", suf, gr(gri), gr(grj), gr(grk), cr(crn)); break;
	}
}

// Load group (major 0x06)
//
// Two addressing modes:
//   • Base + Displacement: ld @(GRi, GRj), GRk  — GRj field is GRj (index)
//   • Base + Immediate:    ld @(GRi, simm12), GRk
// Differentiated by bit [12] per CGEN encoding.
//
// Indexed load (auto-modify variant) is selected when bit [25] is set.

static void decode_load_gr(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	st32 d12 = FRV_SIMM12(insn);
	bool is_imm = (insn >> 12) & 1;
	bool is_postmod = (insn >> 25) & 1; // post-modify (update GRi)

	const char *mn = (subop < 8) ? frv_ls_ops[subop].ld : "??ld";
	const char *pm = is_postmod ? "!" : ""; // '!' denotes post-modify

	if (is_imm) {
		rz_strbuf_appendf(&op->strbuf, "%s%s @(%s, %d), %s", mn, pm, gr(gri), d12, gr(grk));
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s%s @(%s, %s), %s", mn, pm, gr(gri), gr(grj), gr(grk));
	}
}

// ── Store group (major 0x07) ──────────────────────────────────────────────────

static void decode_store_gr(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	st32 d12 = FRV_SIMM12(insn);
	bool is_imm = (insn >> 12) & 1;
	bool is_postmod = (insn >> 25) & 1;

	const char *mn = (subop < 8) ? frv_ls_ops[subop].st : "??st";
	const char *pm = is_postmod ? "!" : "";

	if (is_imm) {
		rz_strbuf_appendf(&op->strbuf, "%s%s %s, @(%s, %d)", mn, pm, gr(grk), gr(gri), d12);
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s%s %s, @(%s, %s)", mn, pm, gr(grk), gr(gri), gr(grj));
	}
}

// ── Branch group (major 0x08) ─────────────────────────────────────────────────
//
// Format (from GCC frv.md and Fujitsu ISA manual):
//   [31:27] = 0x08
//   [26:24] = condition (3-bit for standard conditions)
//   [23:22] = ICCi (integer CC register)
//   [21]    = branch hint (taken=1 / not-taken=0)
//   [20:5]  = label16 (16-bit signed, × 4 to get byte offset)
//   [4:0]   = 0b00000
//
// The 16-bit label is shifted left by 2 (instructions are 4-byte aligned),
// giving a ±128 KB branch range.  Extended branches (24-bit range) use a
// separate major opcode (FRV_MOP_JUMP, 0x09) with CALL encoding.

static void decode_branch(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 cond = FRV_COND(insn);
	ut32 icci = FRV_ICCI(insn);
	bool hint = (insn >> 21) & 1;

	// label16 is bits [20:5], sign-extended, × 4
	st32 lbl16 = (st32)((ut32)FRV_BITS(insn, 20, 5) << 16) >> 14;
	ut64 target = pc + (st64)lbl16;

	const char *mn = frv_bcond_str[cond & 0xF];
	const char *h = hint ? ",t" : ",n"; // branch hint suffix

	if (!mn) {
		rz_strbuf_appendf(&op->strbuf, "??branch.%u", cond);
		return;
	}

	// bra has no ICC operand
	if (cond == FRV_BCOND_RA || cond == FRV_BCOND_NO) {
		rz_strbuf_appendf(&op->strbuf, "%s%s 0x%08" PFMT64x, mn, h, target);
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s%s %s, 0x%08" PFMT64x,
			mn, h, icc(icci), target);
	}

	op->is_branch = true;
	op->target = target;
}

// ── Jump / Call group (major 0x09) ────────────────────────────────────────────
//
// Sub-opcodes:
//   0x0: call  label24   — PC-relative, 24-bit offset × 4, LR ← PC+4
//   0x1: jmpl  @(GRi, GRj) — jump to GRi+GRj, LR ← PC+4
//   0x2: jmpl  @(GRi, simm12)
//   0x3: ret               — return (jmpl @(lr, gr0))
//   0x4: jmpil @(GRi, GRj) — indirect jump without link
//   0x5: jmpil @(GRi, simm12)

static void decode_jump(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	st32 d12 = FRV_SIMM12(insn);

	// call: 24-bit label (bits [23:0] × 4), sign-extended
	st32 lbl24 = (st32)((insn & 0x00FFFFFF) << 8) >> 6;
	ut64 callt = pc + (st64)lbl24;

	switch (subop) {
	case 0: // call label24
		rz_strbuf_appendf(&op->strbuf, "call 0x%08" PFMT64x, callt);
		op->is_call = true;
		op->target = callt;
		break;
	case 1: // jmpl @(GRi, GRj)
		rz_strbuf_appendf(&op->strbuf, "jmpl @(%s, %s)", gr(gri), gr(grj));
		op->is_call = true;
		break;
	case 2: // jmpl @(GRi, simm12)
		rz_strbuf_appendf(&op->strbuf, "jmpl @(%s, %d)", gr(gri), d12);
		op->is_call = true;
		break;
	case 3: // ret — encoded as jmpil @(lr, gr0)
		rz_strbuf_appendf(&op->strbuf, "ret");
		op->is_ret = true;
		break;
	case 4: // jmpil @(GRi, GRj) — no link
		rz_strbuf_appendf(&op->strbuf, "jmpil @(%s, %s)", gr(gri), gr(grj));
		op->is_branch = true;
		break;
	case 5: // jmpil @(GRi, simm12) — no link
		rz_strbuf_appendf(&op->strbuf, "jmpil @(%s, %d)", gr(gri), d12);
		op->is_branch = true;
		break;
	default:
		rz_strbuf_appendf(&op->strbuf, "??jump.%u", subop);
		break;
	}
}

// ── Float arithmetic group (major 0x0A) ───────────────────────────────────────

static void decode_fp_arith(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 fri = FRV_GRI(insn);
	ut32 frj = FRV_GRJ(insn);
	ut32 frk = FRV_GRK(insn);
	const char *mn = (subop < 8) ? frv_fp_arith_mnem[subop] : "??fparith";
	rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s", mn, fr(fri), fr(frj), fr(frk));
}

// ── Float compare group (major 0x0B) ──────────────────────────────────────────

static void decode_fp_cmp(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_BITS(insn, 25, 24);
	ut32 fri = FRV_GRI(insn);
	ut32 frj = FRV_GRJ(insn);
	ut32 fcci = FRV_FCCI(insn);
	const char *mn = (subop < 4) ? frv_fp_cmp_mnem[subop] : "??fpcmp";
	rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s", mn, fr(fri), fr(frj), fcc(fcci));
}

// ── Float branch group (major 0x0C) ───────────────────────────────────────────

static void decode_fp_branch(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 cond = FRV_BITS(insn, 26, 24) & 0x7;
	ut32 fcci = FRV_FCCI(insn);
	st32 lbl16 = (st32)((ut32)FRV_BITS(insn, 20, 5) << 16) >> 14;
	ut64 target = pc + (st64)lbl16;

	const char *mn = frv_fbcond_str[cond];
	if (cond == 7) { // fbra — unconditional
		rz_strbuf_appendf(&op->strbuf, "%s 0x%08" PFMT64x, mn, target);
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s %s, 0x%08" PFMT64x, mn, fcc(fcci), target);
	}
	op->is_branch = true;
	op->target = target;
}

// ── Float load group (major 0x0D) ─────────────────────────────────────────────

static void decode_fp_load(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 frk = FRV_GRK(insn);
	st32 d12 = FRV_SIMM12(insn);
	bool is_imm = (insn >> 12) & 1;

	const char *mn = (subop < 8 && frv_fls_ops[subop].ld)
		? frv_fls_ops[subop].ld
		: "??fldf";

	if (is_imm) {
		rz_strbuf_appendf(&op->strbuf, "%s @(%s, %d), %s", mn, gr(gri), d12, fr(frk));
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s @(%s, %s), %s", mn, gr(gri), gr(grj), fr(frk));
	}
}

// ── Float store group (major 0x0E) ────────────────────────────────────────────

static void decode_fp_store(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 frk = FRV_GRK(insn);
	st32 d12 = FRV_SIMM12(insn);
	bool is_imm = (insn >> 12) & 1;

	const char *mn = (subop < 8 && frv_fls_ops[subop].st)
		? frv_fls_ops[subop].st
		: "??fstf";

	if (is_imm) {
		rz_strbuf_appendf(&op->strbuf, "%s %s, @(%s, %d)", mn, fr(frk), gr(gri), d12);
	} else {
		rz_strbuf_appendf(&op->strbuf, "%s %s, @(%s, %s)", mn, fr(frk), gr(gri), gr(grj));
	}
}

// ── Media instruction group (major 0x0F) ──────────────────────────────────────
//
// FR-V media instructions operate on FR registers as packed 16-bit or 8-bit
// vectors. The sub-opcode space is very large; we decode the most common ones
// and fall back to a hex dump for the rest.
//
// Key media opcodes from GCC frv_init_builtins() and frv.md:
//   mdpackh, mdunpackh, mbtoh, mhtob, maveh, msaths, msathu,
//   maddaccs, msubaccs, mmulhs, mmulhu, ...

static void decode_media(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 fri = FRV_GRI(insn);
	ut32 frj = FRV_GRJ(insn);
	ut32 frk = FRV_GRK(insn);
	// extended sub-opcode sits in bits [11:7]
	ut32 xop = FRV_BITS(insn, 11, 7);

	// Emit a simplified decode; full media decode would need a 256-entry table
	rz_strbuf_appendf(&op->strbuf, "media.%u.%u %s, %s, %s",
		subop, xop, fr(fri), fr(frj), fr(frk));
}

// ── Additional arithmetic with CC output (major 0x10) ─────────────────────────
//
// These are the CC-producing forms that also write a GR result.
// subop 0-3 mirror major 0x00's arithmetic but always produce ICC output.

static void decode_int_arith2(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);
	ut32 cci = FRV_ICCI(insn);

	static const char *mn2[8] = {
		"addcc", "subcc", "smulcc", "umulcc",
		"addxi", "subxi", "??a2_6", "??a2_7"
	};
	rz_strbuf_appendf(&op->strbuf, "%s %s, %s, %s, %s",
		mn2[subop & 0x7], gr(gri), gr(grj), gr(grk), icc(cci));
}

// ── Atomic / swap group (major 0x14) ──────────────────────────────────────────

static void decode_atomic(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	ut32 gri = FRV_GRI(insn);
	ut32 grj = FRV_GRJ(insn);
	ut32 grk = FRV_GRK(insn);

	switch (subop) {
	case 0: // swap @(GRi, GRj), GRk
		rz_strbuf_appendf(&op->strbuf, "swap @(%s, %s), %s", gr(gri), gr(grj), gr(grk));
		break;
	case 1: // cas (compare-and-swap): GRi+GRj if [GRi+GRj]==GRj → GRk
		rz_strbuf_appendf(&op->strbuf, "cas @(%s, %s), %s", gr(gri), gr(grj), gr(grk));
		break;
	default:
		rz_strbuf_appendf(&op->strbuf, "??atomic.%u", subop);
		break;
	}
}

// ── NOP group (major 0x1F) ────────────────────────────────────────────────────
//
// The FR-V has three NOP forms:
//   nop  (integer unit)  = 0x80880000
//   fnop (float unit)    = 0x84880000
//   mnop (media unit)    = 0x8C880000
// They are all major=0x1F with different sub-opcodes.

static void decode_nop(FRVOp *op, ut32 insn, ut64 pc) {
	ut32 subop = FRV_SUBOP(insn);
	switch (subop) {
	case 0: rz_strbuf_append(&op->strbuf, "nop"); break;
	case 1: rz_strbuf_append(&op->strbuf, "fnop"); break;
	case 2: rz_strbuf_append(&op->strbuf, "mnop"); break;
	default:
		rz_strbuf_appendf(&op->strbuf, "nop.%u", subop);
		break;
	}
}

// ── Top-level dispatch ────────────────────────────────────────────────────────
//
// Dispatch on the 5-bit major opcode, identical in philosophy to cil_dis.c's
// opcode_readers_single[] table, but using a switch for clarity.

/**
 * \brief Disassemble one FR-V instruction.
 *
 * \param op    Output structure; strbuf must be pre-initialised.
 * \param buf   Input bytes (big-endian).
 * \param len   Number of available bytes.
 * \param pc    Program counter of this instruction (for branch targets).
 * \return 0 on success, -1 on failure (truncated buffer or unknown opcode).
 */
int frv_disassemble(FRVOp *op, const ut8 *buf, int len, ut64 pc) {
	if (len < FRV_INSN_SIZE) {
		return -1;
	}

	// FR-V is big-endian; read as BE-32
	ut32 insn = rz_read_be32(buf);
	op->raw = insn;

	rz_strbuf_init(&op->strbuf);
	op->target = 0;
	op->is_branch = false;
	op->is_call = false;
	op->is_ret = false;

	ut32 major = FRV_OP(insn);

	switch (major) {
	case FRV_MOP_INT_ARITH: decode_int_arith(op, insn, pc); break;
	case FRV_MOP_INT_LOGIC: decode_int_logic(op, insn, pc); break;
	case FRV_MOP_INT_SHIFT: decode_int_shift(op, insn, pc); break;
	case FRV_MOP_INT_CMP: decode_int_cmp(op, insn, pc); break;
	case FRV_MOP_INT_SET: decode_int_set(op, insn, pc); break;
	case FRV_MOP_INT_COND: decode_int_cond(op, insn, pc); break;
	case FRV_MOP_LOAD_GR: decode_load_gr(op, insn, pc); break;
	case FRV_MOP_STORE_GR: decode_store_gr(op, insn, pc); break;
	case FRV_MOP_BRANCH: decode_branch(op, insn, pc); break;
	case FRV_MOP_JUMP: decode_jump(op, insn, pc); break;
	case FRV_MOP_FP_ARITH: decode_fp_arith(op, insn, pc); break;
	case FRV_MOP_FP_CMP: decode_fp_cmp(op, insn, pc); break;
	case FRV_MOP_FP_BRANCH: decode_fp_branch(op, insn, pc); break;
	case FRV_MOP_FP_LOAD: decode_fp_load(op, insn, pc); break;
	case FRV_MOP_FP_STORE: decode_fp_store(op, insn, pc); break;
	case FRV_MOP_MEDIA: decode_media(op, insn, pc); break;
	case FRV_MOP_INT_ARITH2: decode_int_arith2(op, insn, pc); break;
	case FRV_MOP_ATOMIC: decode_atomic(op, insn, pc); break;
	case FRV_MOP_NOP: decode_nop(op, insn, pc); break;

	default:
		rz_strbuf_appendf(&op->strbuf, "?? 0x%08X", insn);
		// Still return success so the plugin advances past the word
		op->size = FRV_INSN_SIZE;
		return 0;
	}

	op->size = FRV_INSN_SIZE;
	return 0;
}
