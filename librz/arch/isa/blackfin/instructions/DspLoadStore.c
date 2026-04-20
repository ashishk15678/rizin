#include "../include/Instructions.h"
#include "../include/arch_blackfin.h"

bool DisassembleDspLoadStore(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	/* dspLDST
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
	| 1 | 0 | 0 | 1 | 1 | 1 |.W.|.aop...|.m.....|.i.....|.reg.......|
	+---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
	int i = ((instructionWord >> DspLDST_i_bits) & DspLDST_i_mask);
	int m = ((instructionWord >> DspLDST_m_bits) & DspLDST_m_mask);
	int W = ((instructionWord >> DspLDST_W_bits) & DspLDST_W_mask);
	int aop = ((instructionWord >> DspLDST_aop_bits) & DspLDST_aop_mask);
	int reg = ((instructionWord >> DspLDST_reg_bits) & DspLDST_reg_mask);

	// W = 0
	// aop = 00
	// m = 00
	// i = 00
	// reg = 001

	instr->operand_count = 3;

	if (W)
		instr->operation = OP_ST;
	else
		instr->operation = OP_LD;

	InstructionOperand dreg_operand;
	enum Register ireg = iregs(i);
	enum Register mreg;

	InstructionOperand ireg_operand = { .cls = REG, .reg = iregs(i) };

	if (aop == 3) {
		dreg_operand = { .cls = REG, .reg = dregs(reg) };
		ireg = iregs(i);
		mreg = mregs(m);
		instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
		InstructionOperand mem_access_operand = {
			.cls = MEM_ACCESS,
			.mem_access = {
				.mode = MEM_REGREG,
				.ptr_reg = ireg,
				.idx_reg = mreg,
				.oper = OPER_PLUSPLUS,
				.width = 4 }
		};
		if (!W) {
			instr->operands[0] = dreg_operand;
			instr->operands[2] = mem_access_operand;
		} else {
			instr->operands[0] = mem_access_operand;
			instr->operands[2] = dreg_operand;
		}
		return true;
	} else {
		if (m == 0)
			dreg_operand = { .cls = REG, .reg = dregs(reg) };
		if (m == 1)
			dreg_operand = { .cls = REG, .reg = dregs_lo(reg) };
		if (m == 2)
			dreg_operand = { .cls = REG, .reg = dregs_hi(reg) };
	}

	if (!W) {
		switch (aop) {
		case 0:
			instr->operands[0] = dreg_operand;
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REGIMM, .ptr_reg = ireg, .idx_imm = 1, .oper = OPER_PLUSPLUS, .width = m ? 2 : 4 } };
			return true;
		case 1:
			instr->operands[0] = dreg_operand;
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REGIMM, .ptr_reg = ireg, .idx_imm = 1, .oper = OPER_MINUSMINUS, .width = m ? 2 : 4 } };
			return true;
		case 2:
			instr->operands[0] = dreg_operand;
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REG, .ptr_reg = ireg, .width = m ? 2 : 4 } };
			return true;
		default:
			return false;
		}
	} else {
		switch (aop) {
		case 0:
			instr->operands[0] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REGIMM, .ptr_reg = ireg, .idx_imm = 1, .oper = OPER_PLUSPLUS } };
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = dreg_operand;
			return true;
		case 1:
			instr->operands[0] = { .cls = MEM_ACCESS, .mem_access = { .mode = MEM_REGIMM, .ptr_reg = ireg, .idx_imm = 1, .oper = OPER_MINUSMINUS } };
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = dreg_operand;
			return true;
		case 2:
			instr->operands[0] = { .cls = MEM_ACCESS, .mem_access = {
									  .mode = MEM_REG,
									  .ptr_reg = ireg,
								  } };
			instr->operands[1] = { .cls = OPERATOR, .operat = OPER_EQ };
			instr->operands[2] = dreg_operand;
			return true;
		default:
			return false;
		}
	}
	return false;
}
