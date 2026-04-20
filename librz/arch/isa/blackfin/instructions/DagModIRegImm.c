#include "../include/Instructions.h"
#include "../include/arch_blackfin.h"

bool DisassembleDagModIRegImm(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	/* dagMODik
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
	| 1 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 0 | 1 | 1 | 0 |.op....|.i.....|
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
	int i = ((instructionWord >> DagMODik_i_bits) & DagMODik_i_mask);
	int op = ((instructionWord >> DagMODik_op_bits) & DagMODik_op_mask);

	instr->operation = OP_MV;
	instr->operand_count = 3;
	instr->operands[0] = { .cls = REG, .reg = iregs(i) };
	instr->operands[1].cls = OPERATOR;
	instr->operands[2].cls = IMM;
	switch (op) {
	case 0:
	case 2:
		instr->operands[1].operat = OPER_PLUSEQ;
		if (op < 2)
			instr->operands[2].imm = 2;
		else
			instr->operands[2].imm = 4;
		return true;
	case 1:
	case 3:
		instr->operands[1].operat = OPER_MINUSEQ;
		if (op < 2)
			instr->operands[2].imm = 2;
		else
			instr->operands[2].imm = 4;
		return true;
	}
	return false;
}
