#include "../include/Instructions.h"
#include "../include/arch_blackfin.h"

bool DisassembleDagModIRegMReg(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	/* dagMODim
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
	| 1 | 0 | 0 | 1 | 1 | 1 | 1 | 0 |.br| 1 | 1 |.op|.m.....|.i.....|
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
	int i = ((instructionWord >> DagMODim_i_bits) & DagMODim_i_mask);
	int m = ((instructionWord >> DagMODim_m_bits) & DagMODim_m_mask);
	int br = ((instructionWord >> DagMODim_br_bits) & DagMODim_br_mask);
	int op = ((instructionWord >> DagMODim_op_bits) & DagMODim_op_mask);

	instr->operation = OP_MV;
	instr->operand_count = 3;
	instr->operands[0] = { .cls = REG, .reg = iregs(i) };
	instr->operands[1].cls = OPERATOR;
	instr->operands[2] = { .cls = REG, .reg = mregs(m) };

	switch (op) {
	case 0:
		if (br) {
			instr->operation = OP_RCPLUSEQ;
			instr->operand_count = 4;
			instr->operands[3] = { .cls = MNEMOMIC, .mnemonic = OL_BREV };
		} else
			instr->operation = OP_MV;
		instr->operands[1].operat = OPER_PLUSEQ;
		break;
	case 1:
		instr->operation = OP_MINUSEQ;
		instr->operands[1].operat = OPER_MINUSEQ;
		break;
	defaut:
		return false;
	}
	return true;
}
