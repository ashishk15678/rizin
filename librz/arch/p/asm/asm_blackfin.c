/* SPDX-License-Identifier: LGPL-3.0-only */

#include <rz_asm.h>
#include <rz_lib.h>
#include <rz_types.h>
#include <rz_util.h>

static int blackfin_disassemble(RzAsm *a, RzAsmOp *op, const ut8 *buf, int len) {
	rz_return_val_if_fail(a && op && buf, 0);
	rz_strbuf_set(&op->buf_asm, "unimplemented");
	op->size = 0;
	return op->size;
}

static int blackfin_assemble(RzAsm *a, RzAsmOp *op, const char *buf) {
	// RZ_UNUSED(a);
	// RZ_UNUSED(op);
	// RZ_UNUSED(buf);
	return 0;
}

RzAsmPlugin rz_asm_plugin_blackfin = {
	.name = "blackfin",
	.author = "community",
	.version = "0.1.0",
	.arch = "blackfin",
	.license = "LGPL3",
	.bits = 16 | 32,
	.endian = RZ_SYS_ENDIAN_LITTLE,
	.desc = "Blackfin architecture (stub)",
	.disassemble = blackfin_disassemble,
	.assemble = blackfin_assemble,
};
