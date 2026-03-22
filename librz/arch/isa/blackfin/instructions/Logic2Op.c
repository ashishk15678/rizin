#include "../include/Instructions.h"
#include "../include/arch_blackfin.h"

bool DisassembleLogic2Op(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	/* LOGI2op
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
	| 0 | 1 | 0 | 0 | 1 |.opc.......|.src...............|.dst.......|
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
	int src = ((instructionWord >> LOGI2op_src_bits) & LOGI2op_src_mask);
	int opc = ((instructionWord >> LOGI2op_opc_bits) & LOGI2op_opc_mask);
	int dst = ((instructionWord >> LOGI2op_dst_bits) & LOGI2op_dst_mask);

	if (parallel)
		return false;

	if (opc == 0 || opc == 1) {
		instr->operation = OP_CCBITTST;
		instr->operand_count = 6;
		instr->operands[0] = { .cls = REG, .reg = REG_CC };
		instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
		instr->operands[2] = { .cls = MNEMOMIC, .mnemonic = OL_BITTST };
		instr->operands[3] = { .cls = REG, .reg = dregs(dst) };
		instr->operands[4] = { .cls = OPERATOR, .operat = OPER_COMMA };
		instr->operands[5] = { .cls = IMM, .imm = uimm5(src) };
		if (opc == 0)
			instr->operands[2].flags.cc_inverted = true;
		return true;
	} else if (opc == 2 || opc == 3 || opc == 4) {
		instr->operation = OP_BITOP;
		instr->operand_count = 4;
		instr->operands[0].cls = MNEMOMIC;
		instr->operands[1] = { .cls = REG, .reg = dregs(dst) };
		instr->operands[2] = { .cls = OPERATOR, .operat = OPER_COMMA };
		instr->operands[3] = { .cls = IMM, .imm = uimm5(src) };

		if (opc == 2) {
			instr->operands[0].mnemonic = OL_BITSET;
		}
		if (opc == 3) {
			instr->operands[0].mnemonic = OL_BITTGL;
		}
		if (opc == 4) {
			instr->operands[0].mnemonic = OL_BITCLR;
		}
	} else if (opc == 5 || opc == 6 || opc == 7) {
		instr->operand_count = 3;
		instr->operands[0] = { .cls = REG, .reg = dregs(dst) };
		instr->operands[1].cls = OPERATOR;
		instr->operands[2] = { .cls = IMM, .imm = uimm5(src) };

		if (opc == 5) {
			instr->operation = OP_MVSHIFTED;
			instr->operands[1].operat = OPER_ASHIFTREQ;
		} else if (opc == 6) {
			instr->operation = OP_MVSHIFTED;
			instr->operands[1].operat = OPER_LSHIFTREQ;
		} else if (opc == 7) {
			instr->operation = OP_MVSHIFTED;
			instr->operands[1].operat = OPER_LSHIFTLEQ;
		}
		return true;
	}
	return true;
}
