#include "../include/Instructions.h"

bool DisassembleCC2Stat(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	int D = ((instructionWord >> CC2stat_D_bits) & CC2stat_D_mask);
	int op = ((instructionWord >> CC2stat_op_bits) & CC2stat_op_mask);
	int cbit = ((instructionWord >> CC2stat_cbit_bits) & CC2stat_cbit_mask);

	enum Register statbit_id = statbits(cbit);

	if (parallel)
		return false;

	instr->operation = OP_CCSTAT;
	instr->operand_count = 3;
	instr->operands[0].cls = REG;
	instr->operands[1].cls = OPERATOR;
	instr->operands[2].cls = REG;

	if (D) {
		instr->operands[0].reg = statbit_id;
		instr->operands[2].reg = REG_CC;
	} else {
		instr->operands[0].reg = REG_CC;
		instr->operands[2].reg = statbit_id;
	}

	switch (op) {
	case 0:
		instr->operands[1].operat = OPER_EQ;
		break;
	case 1:
		instr->operands[1].operat = OPER_OREQ;
		break;
	case 2:
		instr->operands[1].operat = OPER_ANDEQ;
		break;
	case 3:
		instr->operands[1].operat = OPER_XOREQ;
		break;
	}
	return true;
}
