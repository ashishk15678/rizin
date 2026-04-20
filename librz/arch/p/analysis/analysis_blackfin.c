/* SPDX-License-Identifier: LGPL-3.0-only */

#include <rz_analysis.h>
#include <rz_lib.h>
#include <rz_types.h>
#include <rz_util.h>

static int blackfin_analysis(RzAnalysis *analysis, RzAnalysisOp *op, ut64 addr, const ut8 *data, int len, RzAnalysisOpMask mask) {
	// RZ_UNUSED(analysis);
	// RZ_UNUSED(addr);
	// RZ_UNUSED(data);
	// RZ_UNUSED(mask);

	if (len >= 2) {
		op->size = 2;
	} else if (len > 0) {
		op->size = len;
	} else {
		op->size = 0;
	}
	op->type = RZ_ANALYSIS_OP_TYPE_UNK;
	return op->size;
}

static char *blackfin_get_reg_profile(RzAnalysis *analysis) {
	// RZ_UNUSED(analysis);
	return rz_str_dup(
		"=PC	pc\n"
		"=SP	sp\n"
		"=A0	a0\n"
		"=A1	a1\n"
		"gpr	pc	.32	0	0\n"
		"gpr	sp	.32	4	0\n"
		"gpr	a0	.32	8	0\n"
		"gpr	a1	.32	12	0\n");
}

RzAnalysisPlugin rz_analysis_plugin_blackfin = {
	.name = "blackfin",
	.desc = "Blackfin analysis (stub)",
	.arch = "blackfin",
	.license = "LGPL3",
	.bits = 16 | 32,
	.op = &blackfin_analysis,
	.get_reg_profile = &blackfin_get_reg_profile,
};
