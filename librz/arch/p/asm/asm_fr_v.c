// SPDX-FileCopyrightText: 2026 Ashish Kumar <15678ashishk@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-only

#include <rz_asm.h>
#include <rz_lib.h>
#include "fr-v/fr_v_dis.h"
#include "rz_types_base.h"
#include "rz_util/rz_assert.h"

static int disassemble(const RzAsm *a, RzAsmOp *asm_op, const ut8 *buf, int len) {
	FRVOp op = { 0 };
	if (!a)
		return 0;
	if (frv_disassemble(&op, buf, len, rz_asm_get_pc(a)) != 0) {
		rz_asm_op_set_asm(asm_op, "invalid");
		asm_op->size = FRV_INSN_SIZE;
		return FRV_INSN_SIZE;
	}

	const char *text = rz_strbuf_get(&op.strbuf);
	rz_asm_op_set_asm(asm_op, text ? text : "invalid");
	asm_op->size = op.size;
	rz_strbuf_fini(&op.strbuf);

	return op.size;
}

RzAsmPlugin rz_asm_plugin_fr_v = {
	.name = "frv",
	.arch = "frv",
	.bits = 32,
	.endian = RZ_SYS_ENDIAN_BIG,
	.desc = "Fujitsu FR-V RISC-VLIW disassembler",
	.license = "LGPL-3",
	.author = "contributors",
	.version = "0.1.0",
	.disassemble = disassemble,
	.assemble = NULL,
};

#ifndef CORELIB
RZ_API RzLibStruct rizin_plugin = {
	.type = RZ_LIB_TYPE_ASM,
	.data = &rz_asm_plugin_fr_v,
	.version = RZ_VERSION,
};
#endif
