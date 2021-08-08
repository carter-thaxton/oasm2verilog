
#ifndef ALU_H
#define ALU_H

// The ALU is a silicon object, with very special support within the OASM language.

#include "SiliconObject.h"
#include "TF.h"
#include "Module.h"
#include "Signal.h"
#include "Parameter.h"
#include "AluFunction.h"
#include "SymbolTable.h"

#define MAX_ALU_INSTRUCTIONS    (8)

#define MAX_ALU_WORD_REGS		(9)
#define MAX_ALU_WORD_INS		(13)
#define MAX_ALU_CARRY_INS		(13)
#define MAX_ALU_BRANCHES		(2)
#define MAX_ALU_CONSTANTS		(2)
#define MAX_ALU_NN_REGS			(4)

#define ALU_TF_REG_OFFSET		(0)
#define ALU_WORD_REG_OFFSET		(10)
#define ALU_BRANCH_REG_OFFSET   (20)
#define ALU_COND_BYPASS_REG		(22)
#define ALU_COND_UPDATE_REG		(23)
#define ALU_WORD_IN_OFFSET	    (30)
#define ALU_CARRY_IN_OFFSET	    (50)
#define ALU_CONSTANT_REG_OFFSET	(70)
#define ALU_BUILTIN_0_REG      (80)
#define ALU_BUILTIN_1_REG      (81)
#define ALU_BUILTIN_FFFF_REG   (82)


// Register Numbers:
// 0-3    - bit regs
// 10-18  - word regs
// 20-21  - branches
// 22     - cond_bypass
// 23     - cond_update
// 30-43  - word ins
// 50-63  - carry ins
// 70-71  - constants   (also assigned to word ins)
// 80     - 0x0000      (built-in constant)
// 81     - 0x0001      (built-in constant)
// 82     - 0xFFFF      (built-in constant)

class AluInstruction;

class Alu : public TF_Module
{
public:
	Alu(const char *name = NULL, Module *parent = NULL);
	virtual ~Alu();

	// Provide definition on instances as well as via a static method
	virtual const SiliconObjectDefinition *Definition() const;
	static  const SiliconObjectDefinition *BuiltinDefinition();

	virtual void Print(FILE *f) const;

	AluInstruction *AddInstruction(AluInstruction *inst);
	Signal *AddTFA(Signal *dest, const TruthFunction &tf);

	Signal *SignalFromRegisterNumber(int n) const;
	static const char *RegisterNameFromRegisterNumber(int n);

	// Defined in Alu_GenerateVerilog
	virtual void GenerateVerilog(FILE *f) const;
	void GenerateVerilogMappingComment(FILE *f) const;

protected:
	// Calls made after parsing module
	virtual bool AssignResources();
	bool SetupBranches();

private:
	AluInstruction *instructions[MAX_ALU_INSTRUCTIONS];
	int num_instructions;

	Signal *word_regs[MAX_ALU_WORD_REGS];
	int num_word_regs;

	Signal *word_ins[MAX_ALU_WORD_INS];
	int num_word_ins;

	Signal *carry_ins[MAX_ALU_CARRY_INS];
	int num_carry_ins;

	Signal *branches[MAX_ALU_BRANCHES];
	TruthFunction branch_logic[MAX_ALU_BRANCHES];
	int num_branches;

	Signal *cond_bypass;
	TruthFunction cond_bypass_logic;

	Signal *cond_update;
	TruthFunction cond_update_logic;

	Signal *constants[MAX_ALU_CONSTANTS];
	int num_constants;

	Signal *builtin0;
	Signal *builtin1;
	Signal *builtinFFFF;

	// Singleton instance for built-in special definition, created on-demand
	static SiliconObjectDefinition *definition;
};


#endif
