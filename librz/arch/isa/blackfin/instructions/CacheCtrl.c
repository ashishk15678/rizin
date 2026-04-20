#include "../include/Instructions.h"

bool DisassembleCacheCtrl(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	int a = ((instructionWord >> CaCTRL_a_bits) & CaCTRL_a_mask);
	int op = ((instructionWord >> CaCTRL_op_bits) & CaCTRL_op_mask);
	int reg = ((instructionWord >> CaCTRL_reg_bits) & CaCTRL_reg_mask);

	if (parallel)
		return false;

	instr->operands[0].cls = MNEMOMIC;
	instr->operands[1] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REG, .ptr_reg = pregs(reg), .width = 4 } };
	instr->operation = OP_CACHE;

	switch (op) {
	case 0:
		instr->operands[0].mnemonic = OL_PREFETCH;
		break;
	case 1:
		instr->operands[0].mnemonic = OL_FLUSHINV;
		break;
	case 2:
		instr->operands[0].mnemonic = OL_FLUSH;
		break;
	case 3:
		instr->operands[0].mnemonic = OL_IFLUSH;
		break;
	}

	if (a) {
		instr->operands[1].mem_access.mode = MEM_REGIMM;
		instr->operands[1].mem_access.idx_imm = 1;
	}

	return true;
}
