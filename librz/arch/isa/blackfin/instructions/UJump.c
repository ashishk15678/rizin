#include "../include/Instructions.h"
#include "../include/arch_blackfin.h"

bool DisassembleUJump(uint16_t instructionWord, struct Instruction *instr, bool parallel) {
	int32_t offset = ((instructionWord >> UJump_offset_bits) & UJump_offset_mask);

	if (parallel)
		return false;

	instr->operation = OP_JMPREL;
	instr->operand_count = 2;
	instr->operands[0] = { .cls = MNEMOMIC, .mnemonic = OL_JUMPS };
	instr->operands[1] = { .cls = IMM, .imm = pcrel12(offset) };
	instr->operands[1].flags.pcrelative = true;

	return true;
}
