
#include "Alu.h"
#include "AluInstruction.h"
#include "EnumValue.h"
#include "Common.h"

Alu::Alu(const char *name, Module *parent)
	: TF_Module(name, parent), instructions(), num_instructions(0),
	  word_regs(), num_word_regs(0),
	  word_ins(), num_word_ins(0),
	  carry_ins(), num_carry_ins(0),
	  branches(), num_branches(0),
	  cond_bypass(0), cond_update(0),
	  constants(), num_constants(0)
{
}

Alu::~Alu()
{
	// Delete ALU instructions allocated on heap
	for (int i=0; i < num_instructions; i++)
	{
		delete instructions[i];
	}
}

AluInstruction *Alu::AddInstruction(AluInstruction *inst)
{
	// Check for too many instructions
	if (num_instructions >= MAX_ALU_INSTRUCTIONS)
	{
		yyerrorf("Only %d instructions are allowed in an ALU", MAX_ALU_INSTRUCTIONS);
		return NULL;
	}

	// Check for instructions with duplicated labels, unless anonymous
	if (inst->label)
	{
		for (int i=0; i < num_instructions; i++)
		{
			if (instructions[i]->label)
			{
				if (strcmp(inst->label, instructions[i]->label) == 0)
				{
					yyerrorf("Multiple instructions with label '%s'", inst->label);
					return NULL;
				}
			}
		}
	}

	// Assign word_ins and carry_ins as they are used in instructions
	if (inst->fcn.Fcn)
	{
		int nargs = inst->fcn.Fcn->NumArgs();
		for (int i=0; i < nargs; i++)
		{
			// Skip immediate values in instructions
			if (inst->fcn.Fcn->ArgType(i) != 'k')
			{
				Signal *arg = inst->fcn.Args[i].sig;
				if (arg->RegisterNumber < 0)
				{
					if (arg->Behavior == BEHAVIOR_WIRE || arg->Behavior == BEHAVIOR_DELAY)
					{
						if (arg->DataType == DATA_TYPE_WORD)
						{
							if (num_word_ins >= MAX_ALU_WORD_INS)
							{
								yyerrorf("Too many word inputs in ALU.  At most %d word inputs are allowed", MAX_ALU_WORD_INS);
								return NULL;
							}
							word_ins[num_word_ins] = arg;
							arg->RegisterNumber = ALU_WORD_IN_OFFSET + num_word_ins;
							num_word_ins++;
							continue;
						}
						else if (arg->DataType == DATA_TYPE_BIT)
						{
							if (num_carry_ins >= MAX_ALU_CARRY_INS)
							{
								yyerrorf("Too many carry inputs in ALU.  At most %d carry inputs are allowed", MAX_ALU_CARRY_INS);
								return NULL;
							}
							carry_ins[num_carry_ins] = arg;
							arg->RegisterNumber = ALU_CARRY_IN_OFFSET + num_carry_ins;
							num_carry_ins++;
							continue;
						}
					}
				}
			}
		}
	}

	// Also assign any latched wires as word_ins
	for (int i=0; i < inst->num_latches; i++)
	{
		Signal *latch = inst->latches[i];
		if (latch->RegisterNumber < 0)
		{
			if (num_word_ins >= MAX_ALU_WORD_INS)
			{
				yyerrorf("Too many word inputs in ALU.  At most %d word inputs are allowed", MAX_ALU_WORD_INS);
				return NULL;
			}
			word_ins[num_word_ins] = latch;
			latch->RegisterNumber = ALU_WORD_IN_OFFSET + num_word_ins;
			num_word_ins++;
		}
		else
		{
			// If already assigned, check that it is a word_in
			if (latch->RegisterNumber < ALU_WORD_IN_OFFSET || latch->RegisterNumber >= ALU_WORD_IN_OFFSET + MAX_ALU_WORD_INS)
			{
				yyerrorfl(inst->Location, "Illegal signal used in latch '%s'.  Only word_in signals may be latched", latch->Name());
				return NULL;
			}
		}
	}

	// Add instruction to list, and set instruction index
	instructions[num_instructions] = inst;
	inst->index = num_instructions;
	num_instructions++;

	return inst;
}


// Resource allocation is performed as TFAs are added
Signal *Alu::AddTFA(Signal *dest, const TruthFunction &tf)
{
	// Check whether dest is declared
	bool found = false;
	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		if (GetSignal(i) == dest)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		yyerrorf("TFA bit register '%s' not declared", dest->Name());
		return NULL;
	}

	// Check behavior and data type
	if (dest->DataType != DATA_TYPE_BIT)
	{
		yyerrorf("TFA register '%s' must be declared as a bit", dest->Name());
		return NULL;
	}

	if (dest->Behavior != BEHAVIOR_BRANCH && dest->Behavior != BEHAVIOR_COND_BYPASS && dest->Behavior != BEHAVIOR_COND_UPDATE)
	{
		yyerrorf("TFA register '%s' must be declared as branch, cond_bypass, or cond_update", dest->Name());
		return NULL;
	}

	// Check whether already assigned
	if (dest->RegisterNumber >= 0)
	{
		yyerrorf("TFA register '%s' has already been assigned", dest->Name());
		return NULL;
	}

	if (dest->Behavior == BEHAVIOR_BRANCH)
	{
		if (num_branches >= MAX_ALU_BRANCHES)
		{
			yyerrorf("Too many branch registers in ALU TFA.  Only %d branches are allowed", MAX_ALU_BRANCHES);
			return NULL;
		}
		branches[num_branches] = dest;
		branch_logic[num_branches] = tf;
		dest->RegisterNumber = ALU_BRANCH_REG_OFFSET + num_branches;
		num_branches++;
	}
	else if (dest->Behavior == BEHAVIOR_COND_BYPASS)
	{
		if (cond_bypass)
		{
			yyerrorf("cond_bypass already declared in ALU.  Only 1 cond_bypass is allowed");
			return NULL;
		}
		cond_bypass = dest;
		cond_bypass_logic = tf;
		dest->RegisterNumber = ALU_COND_BYPASS_REG;
	}
	else if (dest->Behavior == BEHAVIOR_COND_UPDATE)
	{
		if (cond_update)
		{
			yyerrorf("cond_update already declared in ALU.  Only 1 cond_update is allowed");
			return NULL;
		}
		cond_update = dest;
		cond_update_logic = tf;
		dest->RegisterNumber = ALU_COND_UPDATE_REG;
	}

	return dest;
}


void Alu::Print(FILE *f) const
{
	// Handles signals and TFs
	TF_Module::Print(f);

	// TFA
	if (num_branches > 0 || cond_update || cond_bypass)
	{
		fprintf(f, "\tTFA\n\t{\n");
		for (int i=0; i < num_branches; i++)
		{
			fprintf(f, "\t\t%s = ", branches[i]->Name());
			branch_logic[i].Print(f);
			fprintf(f, "\n");
		}
		if (cond_bypass)
		{
			fprintf(f, "\t\t%s = ", cond_bypass->Name());
			cond_bypass_logic.Print(f);
			fprintf(f, "\n");
		}
		if (cond_update)
		{
			fprintf(f, "\t\t%s = ", cond_update->Name());
			cond_update_logic.Print(f);
			fprintf(f, "\n");
		}
		fprintf(f, "\t}\n");
	}

	// Instructions
	if (num_instructions > 0)
	{
		fprintf(f, "\tinst\n");
		fprintf(f, "\t{\n");
		for (int i=0; i < num_instructions; i++)
		{
			fprintf(f, "\t\t");
			instructions[i]->Print(f);
		}
		fprintf(f, "\t}\n");
	}
}


// Register Numbers:
// 0-3    - bit regs
// 10-18  - word regs
// 20-21  - branches
// 22     - cond_bypass
// 23     - cond_update
// 30-43  - word ins
// 50-63  - carry ins
// 70-71  - constants  (also assigned to word ins)
Signal *Alu::SignalFromRegisterNumber(int n) const
{
	// tfN_reg
	if (n < ALU_TF_REG_OFFSET)
		return NULL;

	if (n < ALU_TF_REG_OFFSET + MAX_TFS)
		return tf_regs[n - ALU_TF_REG_OFFSET];

	// wordN_reg
	if (n < ALU_WORD_REG_OFFSET)
		return NULL;

	if (n < ALU_WORD_REG_OFFSET + MAX_ALU_WORD_REGS)
		return word_regs[n - ALU_WORD_REG_OFFSET];

	// branchN
	if (n < ALU_BRANCH_REG_OFFSET)
		return NULL;

	if (n < ALU_BRANCH_REG_OFFSET + MAX_ALU_BRANCHES)
		return branches[n - ALU_BRANCH_REG_OFFSET];

	// cond_bypass
	if (n == ALU_COND_BYPASS_REG)
		return cond_bypass;

	// cond_update
	if (n == ALU_COND_UPDATE_REG)
		return cond_update;

	// wordN_in
	if (n < ALU_WORD_IN_OFFSET)
		return NULL;

	if (n < ALU_WORD_IN_OFFSET + MAX_ALU_WORD_INS)
		return word_ins[n - ALU_WORD_IN_OFFSET];

	// carryN_in
	if (n < ALU_CARRY_IN_OFFSET)
		return NULL;

	if (n < ALU_CARRY_IN_OFFSET + MAX_ALU_CARRY_INS)
		return carry_ins[n - ALU_CARRY_IN_OFFSET];

	// constN - also assigned to wordN_in
	if (n < ALU_CONSTANT_REG_OFFSET)
		return NULL;

	if (n < ALU_CONSTANT_REG_OFFSET + MAX_ALU_CONSTANTS)
		return constants[n - ALU_CONSTANT_REG_OFFSET];

	// builtin constants
	if (n == ALU_BUILTIN_0_REG)
		return builtin0;

	if (n == ALU_BUILTIN_1_REG)
		return builtin1;

	if (n == ALU_BUILTIN_FFFF_REG)
		return builtinFFFF;

	return NULL;
}

// Returns a string name for the register number
const char *Alu::RegisterNameFromRegisterNumber(int n)
{
	static char result[32];

	// tfN_reg
	if (n < ALU_TF_REG_OFFSET)
		return NULL;

	if (n < ALU_TF_REG_OFFSET + MAX_TFS)
	{
		sprintf(result, "tf%d_reg", n - ALU_TF_REG_OFFSET);
		return result;
	}

	// wordN_reg
	if (n < ALU_WORD_REG_OFFSET)
		return NULL;

	if (n < ALU_WORD_REG_OFFSET + MAX_ALU_WORD_REGS)
	{
		sprintf(result, "word%d_reg", n - ALU_WORD_REG_OFFSET);
		return result;
	}

	// branchN
	if (n < ALU_BRANCH_REG_OFFSET)
		return NULL;

	if (n < ALU_BRANCH_REG_OFFSET + MAX_ALU_BRANCHES)
	{
		sprintf(result, "branch%d", n - ALU_BRANCH_REG_OFFSET);
		return result;
	}

	// cond_bypass
	if (n == ALU_COND_BYPASS_REG)
		return "cond_bypass";

	// cond_update
	if (n == ALU_COND_UPDATE_REG)
		return "cond_update";

	// wordN_in
	if (n < ALU_WORD_IN_OFFSET)
		return NULL;

	if (n < ALU_WORD_IN_OFFSET + MAX_ALU_WORD_INS)
	{
		sprintf(result, "word%d_in", n - ALU_WORD_IN_OFFSET);
		return result;
	}

	// carryN_in
	if (n < ALU_CARRY_IN_OFFSET)
		return NULL;

	if (n < ALU_CARRY_IN_OFFSET + MAX_ALU_CARRY_INS)
	{
		sprintf(result, "carry%d_in", n - ALU_CARRY_IN_OFFSET);
		return result;
	}

	// constN - also assigned to wordN_in
	if (n < ALU_CONSTANT_REG_OFFSET)
		return NULL;

	if (n < ALU_CONSTANT_REG_OFFSET + MAX_ALU_CONSTANTS)
	{
		sprintf(result, "const%d", n - ALU_CONSTANT_REG_OFFSET);
		return result;
	}

	// builtin constants
	if (n == ALU_BUILTIN_0_REG)
		return "builtin0";

	if (n == ALU_BUILTIN_1_REG)
		return "builtin1";

	if (n == ALU_BUILTIN_FFFF_REG)
		return "builtinFFFF";

	return NULL;
}

// Assign signals to word_regs, constants, tf_regs, branches, cond_bypass, cond_update
// Generate errors as appropriate
// Returns true if successful
bool Alu::AssignResources()
{
	bool ok = true;
	int n = SignalCount();


	// Check for word registers that are constant, because they are never the destination of an instruction,
	// and never have a bit-slice taken of them (v-bit or bits 0-3), and promote them to BEHAVIOR_CONST.
	// These optimized registers should ignore warm_reset.
	for (int i=0; i < n; i++)
	{
		Signal *sig = GetSignal(i);
		if (sig->Behavior == BEHAVIOR_REG && sig->DataType == DATA_TYPE_WORD && sig->RegisterNumber < 0)
		{
			bool usedAsDest = false;
			for (int j=0; j < num_instructions && !usedAsDest; j++)
			{
				AluInstruction *inst = instructions[j];
				for (int d=0; d < inst->num_dests && !usedAsDest; d++)
				{
					if (sig == inst->dests[d])
						usedAsDest = true;
				}
			}

			bool usesBitSlice = false;
			for (int j=0; j < n && !usesBitSlice; j++)
			{
				Signal *tmp = GetSignal(j);
				if (tmp->Behavior == BEHAVIOR_BIT_SLICE && tmp->BaseSignal == sig)
					usesBitSlice = true;
			}

			if (!usedAsDest && !usesBitSlice)
			{
				// Promote to const, and do not use warm reset
				sig->Behavior = BEHAVIOR_CONST;
				sig->UsesWarmReset = false;
			}
		}
	}


	// Assign registers and constants to resources such as wordN_reg, tfN_reg, wordN_in, constN
	for (int i=0; i < n; i++)
	{
		Signal *sig = GetSignal(i);

		// Only assign unassigned registers
		if (sig->RegisterNumber < 0)
		{
			if (sig->Behavior == BEHAVIOR_REG)
			{
				if (sig->DataType == DATA_TYPE_WORD)
				{
					if (num_word_regs >= MAX_ALU_WORD_REGS)
					{
						yyerrorfl(Location, "Too many word registers in ALU.  Only %d registers are allowed", MAX_ALU_WORD_REGS);
						ok = false;
						continue;
					}
					word_regs[num_word_regs] = sig;
					sig->RegisterNumber = ALU_WORD_REG_OFFSET + num_word_regs;
					num_word_regs++;
				}
				else if (sig->DataType == DATA_TYPE_BIT)
				{
					if (num_tfs >= MAX_TFS)
					{
						yyerrorfl(Location, "Too many bit registers in ALU.  Only %d TFs are allowed", MAX_TFS);
						ok = false;
						continue;
					}

					// Unassigned TFs should hold their previous value
					tf_regs[num_tfs] = sig;
					tf_logic[num_tfs] = TruthFunction::FromSignal(sig);
					sig->RegisterNumber = ALU_TF_REG_OFFSET + num_tfs;
					num_tfs++;
				}
			}

			else if (sig->Behavior == BEHAVIOR_CONST)
			{
				if (sig->DataType == DATA_TYPE_WORD)
				{
					// If a previously assigned constant has the same value, share the register
					for (int j=0; j < n; j++)
					{
						Signal *sigcmp = GetSignal(j);
						if ((sigcmp->Behavior == BEHAVIOR_CONST) &&
							(sigcmp->DataType == DATA_TYPE_WORD) &&
							(sigcmp->InitialValue == sig->InitialValue) &&
							(sigcmp->RegisterNumber >= 0))
						{
							sig->RegisterNumber = sigcmp->RegisterNumber;
							break;
						}
					}
					if (sig->RegisterNumber >= 0)
						continue;


					// Handle built-in constants  (0, 1, 0xFFFF)
					if (sig->InitialValue == 0)
					{
						builtin0 = sig;
						sig->RegisterNumber = ALU_BUILTIN_0_REG;
					}
					else if (sig->InitialValue == 1)
					{
						builtin1 = sig;
						sig->RegisterNumber = ALU_BUILTIN_1_REG;
					}
					else if (sig->InitialValue == 0xFFFF)
					{
						builtinFFFF = sig;
						sig->RegisterNumber = ALU_BUILTIN_FFFF_REG;
					}

					// Use constants if possible
					else if (num_constants >= MAX_ALU_CONSTANTS)
					{
						// If out of word constants, use a word reg
						if (num_word_regs >= MAX_ALU_WORD_REGS)
						{
							yyerrorfl(Location, "Too many word registers in ALU.  All constants used, and only %d other registers are allowed", MAX_ALU_WORD_REGS);
							ok = false;
							continue;
						}
						word_regs[num_word_regs] = sig;
						sig->RegisterNumber = ALU_WORD_REG_OFFSET + num_word_regs;
						num_word_regs++;
					}

					// Use nearest-neighbor registers when no more constants available
					else
					{
						// Use constant, implemented using a word_in
						if (num_word_ins >= MAX_ALU_WORD_INS)
						{
							yyerrorfl(Location, "Too many word inputs in ALU.  Cannot provide constant input.  At most %d word inputs are allowed", MAX_ALU_WORD_INS);
							ok = false;
							continue;
						}
						constants[num_constants] = sig;
						word_ins[num_word_ins] = sig;
						sig->RegisterNumber = ALU_WORD_IN_OFFSET + num_word_ins;	// assign to word_in register number
						num_constants++;
						num_word_ins++;
					}
				}

				else if (sig->DataType == DATA_TYPE_BIT)
				{
					// If a previously assigned constant has the same value, share the register
					for (int j=0; j < n; j++)
					{
						Signal *sigcmp = GetSignal(j);
						if ((sigcmp->Behavior == BEHAVIOR_CONST) &&
							(sigcmp->DataType == DATA_TYPE_BIT) &&
							(sigcmp->InitialValue == sig->InitialValue) &&
							(sigcmp->RegisterNumber >= 0))
						{
							sig->RegisterNumber = sigcmp->RegisterNumber;
							break;
						}
					}
					if (sig->RegisterNumber >= 0)
						continue;


					// bit constants use TFs
					if (num_tfs >= MAX_TFS)
					{
						yyerrorfl(Location, "Too many bit registers in ALU.  Only %d TFs are allowed", MAX_TFS);
						ok = false;
						continue;
					}

					// Constant TFs should hold their previous value
					tf_regs[num_tfs] = sig;
					tf_logic[num_tfs] = TruthFunction::FromSignal(sig);
					sig->RegisterNumber = ALU_TF_REG_OFFSET + num_tfs;
					num_tfs++;
				}
			}

		}
	}

	// Count the number of word registers that need initialized values other than 0, or use warm_reset, or have a bit-slice 0-3 taken of them.
	int nn_regs = 0;
	for (int i=0; i < num_word_regs; i++)
	{
		Signal *reg = word_regs[i];

		// Test whether another signal refers to this reg using a non v-bit slice
		bool has_bit_slice = false;
		for (int j=0; j < n; j++)
		{
			Signal *sig = GetSignal(j);
			if (sig->Behavior == BEHAVIOR_BIT_SLICE && sig->BaseSignal == reg && sig->BitSliceIndex != V_BIT_SLICE_INDEX)
			{
				has_bit_slice = true;
				break;
			}
		}

		if (has_bit_slice || reg->InitialValue != 0 || reg->UsesWarmReset)
			nn_regs++;
	}

	if (nn_regs > MAX_ALU_NN_REGS)
	{
		yyerrorfl(Location, "Only %d nearest-neighbor word regs are available, but %d are non-zero, use warm_reset, or use bit-slices.", MAX_ALU_NN_REGS, nn_regs);
		return false;
	}

	// Lookup and set branch indexes based on string labels
	if (!SetupBranches())
		return false;

	return ok;
}

bool Alu::SetupBranches()
{
	bool ok = true;

	// Associate branch labels with instruction numbers, and provide appropriate errors
	for (int i=0; i < num_instructions; i++)
	{
		AluInstruction *inst = instructions[i];

		// Determine branch register number (0-1) for each branch condition
		int c0 = -1;
		if (inst->branch_conds[0])
		{
			if (inst->branch_conds[0] == branches[0])
				c0 = 0;
			else if (inst->branch_conds[0] == branches[1])
				c0 = 1;
			else
			{
				yyerrorfl(inst->Location, "Instruction %d uses an unassigned branch condition", i);
				ok = false;
			}
		}

		int c1 = -1;
		if (inst->branch_conds[1])
		{
			if (inst->branch_conds[1] == branches[0])
				c1 = 0;
			else if (inst->branch_conds[1] == branches[1])
				c1 = 1;
			else
			{
				yyerrorfl(inst->Location, "Instruction %d uses an unassigned branch condition", i);
				ok = false;
			}
		}

		// Check if branch condition registers are encoded in the instruction in the opposite order as in the TFA
		if (c0 == 1)
		{
			// Swap branch 0/1, which means swapping the branch condition registers and branch labels 1,2 (leave 0,3 as is)
			Signal *cond_temp = inst->branch_conds[0];
			inst->branch_conds[0] = inst->branch_conds[1];
			inst->branch_conds[1] = cond_temp;

			const char *label_temp = inst->branch_labels[1];
			inst->branch_labels[1] = inst->branch_labels[2];
			inst->branch_labels[2] = label_temp;
		}


		// Assign branch labels to branch indexes
		for (int b=0; b < 4; b++)
		{
			if (inst->branch_labels[b] == NULL)
			{
				// No branch label means to implicitly goto the next instruction modulo num_instructions.
				// Note that this is somewhat different from next mod MAX_ALU_INSTRUCTIONS (8)
				// Two benefits come of this:
				// - The ALU programming model is independent of the maximum number of instructions
				// - Single-instruction ALUs require just a single instruction with no goto
				inst->branch_indexes[b] = (i+1) % num_instructions;
			}
			else
			{
				// Lookup branch label, and error if it does not exist
				bool found = false;
				for (int j=0; j < num_instructions; j++)
				{
					if (instructions[j]->label)
					{
						if (strcmp(inst->branch_labels[b], instructions[j]->label) == 0)
						{
							inst->branch_indexes[b] = j;
							found = true;
							break;
						}
					}
				}
				if (!found)
				{
					yyerrorfl(inst->Location, "Instruction label '%s' not found", inst->branch_labels[b]);
					inst->branch_indexes[b] = -1;        // Mark as invalid instruction destination
					ok = false;
					break;
				}
			}
		}
	}

	return ok;
}



/*
 * Singleton instance of definition, created on-demand
 */

// Factory method to create ALU
static SiliconObject *AluFactory(const SiliconObjectDefinition *definition, const char *name, Module *parent)
{
	return new Alu(name, parent);
}

const SiliconObjectDefinition *Alu::Definition() const
{
	// Use built-in static method
	return BuiltinDefinition();
}

const SiliconObjectDefinition *Alu::BuiltinDefinition()
{
	// Return singleton instance if already created
	if (definition)
		return definition;

	// Create a new builtin singleton symbol table instance
	SymbolTable *symbols = new SymbolTable(true);


	// ALU Signals

	// These signals cannot be used outside of the ALU
	symbols->Add(new Signal("status",       BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_NONE ));
	symbols->Add(new Signal("carry",        BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_NONE ));
	symbols->Add(new Signal("zero",         BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_NONE ));
	symbols->Add(new Signal("negative",     BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_NONE ));
	symbols->Add(new Signal("overflow",     BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_NONE ));

	// These signals can be connected to as inputs
	symbols->Add(new Signal("v_in",         BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("warm_reset",   BEHAVIOR_BUILTIN,  DATA_TYPE_BIT,  DIR_IN ));


	// ALU Enumerations
	// hold is a valid RHS for v_out in ALU instruction.
	// It is not added to the global symbol table like many enum values, because it is only valid in the "inst" section of an ALU.
	symbols->Add(new EnumValue("hold"));

	// ALU Functions   (Some functions are repeated with varying numbers of arguments)
	symbols->Add(new AluFunction("add",     "ww"));
	symbols->Add(new AluFunction("hadd",    "wwb"));
	symbols->Add(new AluFunction("and",     "ww"));
	symbols->Add(new AluFunction("and",     "www"));
	symbols->Add(new AluFunction("avg",     "ww"));
	symbols->Add(new AluFunction("havg",    "wwb"));
	symbols->Add(new AluFunction("bmux",    "www"));
	symbols->Add(new AluFunction("cmp",     "ww"));
	symbols->Add(new AluFunction("hcmp",    "wwb"));
	symbols->Add(new AluFunction("dec",     "w"));
	symbols->Add(new AluFunction("hdec",    "wb"));
	symbols->Add(new AluFunction("dshl",    "www"));
	symbols->Add(new AluFunction("dshli",   "wwk"));
	symbols->Add(new AluFunction("dshri",   "wwk"));
	symbols->Add(new AluFunction("inc",     "w"));
	symbols->Add(new AluFunction("hinc",    "wb"));
	symbols->Add(new AluFunction("inv",     "w"));
	symbols->Add(new AluFunction("mov",     "w"));
	symbols->Add(new AluFunction("mux",     "ww"));
	symbols->Add(new AluFunction("neg",     "w"));
	symbols->Add(new AluFunction("hneg",    "wb"));
	symbols->Add(new AluFunction("or",      "ww"));
	symbols->Add(new AluFunction("or",      "www"));
	symbols->Add(new AluFunction("roli",    "wk"));
	symbols->Add(new AluFunction("rol",     "ww"));
	symbols->Add(new AluFunction("rori",    "wk"));
	symbols->Add(new AluFunction("setsb",   ""));
	symbols->Add(new AluFunction("shli",    "wk"));
	symbols->Add(new AluFunction("shl",     "w"));
	symbols->Add(new AluFunction("shri",    "wk"));
	symbols->Add(new AluFunction("sub",     "ww"));
	symbols->Add(new AluFunction("hsub",    "wb"));
	symbols->Add(new AluFunction("xor",     "ww"));
	symbols->Add(new AluFunction("xor",     "www"));
	symbols->Add(new AluFunction("xoradd",  "www"));
	symbols->Add(new AluFunction("hxoradd", "wwbw"));
	symbols->Add(new AluFunction("xoravg",  "www"));
	symbols->Add(new AluFunction("hxoravg", "wwbw"));
	symbols->Add(new AluFunction("xorcmp",  "www"));
	symbols->Add(new AluFunction("hxorcmp", "wwbw"));
	symbols->Add(new AluFunction("zero",    ""));           // Note: does not conflict with signal "zero", because functions have a mangled name.


	// Create a new singleton definition
	definition = new SiliconObjectDefinition("ALU", symbols, AluFactory);

	return definition;
}

SiliconObjectDefinition *Alu::definition = NULL;
