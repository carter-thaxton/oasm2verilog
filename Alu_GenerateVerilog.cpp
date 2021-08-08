
#include "Alu.h"
#include "AluInstruction.h"
#include "Common.h"

#define TF_STRING_BUFFER_LENGTH     (1024)

// Returns a string such as "word0_reg", "word2_in", "tf0_reg", "add_carry", or even just "1"
static const char *ToInstructionString(Signal *sig)
{
	int regnum = sig->RegisterNumber;
	if (regnum < 0)
	{
		return sig->Name();
	}
	else if (regnum == ALU_BUILTIN_0_REG)
	{
		return "0";
	}
	else if (regnum == ALU_BUILTIN_1_REG)
	{
		return "1";
	}
	else if (regnum == ALU_BUILTIN_FFFF_REG)
	{
		return "0xffff";
	}
	else
	{
		const char *result = Alu::RegisterNameFromRegisterNumber(regnum);
		if (result)
		{
			return result;
		}
		else
		{
			yyerrorf("Unable to convert register %d to appropriate string in ALU instruction", regnum);
			return "UNKNOWN";
		}
	}
}


// Generate a Verilog comment that contains
// the signal-to-resource mapping, such as Count -> word0_reg
void Alu::GenerateVerilogMappingComment(FILE *f) const
{
	int n = SignalCount();

	for (int i=0; i < n; i++)
	{
		Signal *sig = GetSignal(i);

		const char *name = sig->Name();

		const char *reg;
		if (sig->RegisterNumber >= 0)
			reg = RegisterNameFromRegisterNumber(sig->RegisterNumber);
		else
			reg = "none";

		F2("// %s\t\t%s\n", name, reg);
	}
	F0("//\n");
}


void Alu::GenerateVerilog(FILE *f) const
{
	F0("\n");

	F0("//\n");
	F2("// ================ %s - %s ================\n", ModuleTypeName(), Name());
	F0("//\n");

	GenerateVerilogMappingComment(f);

	int n = SignalCount();

	// module ModuleName (
	//     input  [16:0] InWord1,
	//     input         InBit2,
	//     output [16:0] OutWord3
	// );

	F0("module ");
	GenerateVerilogModuleName(f);
	F0(" (");

	bool first = true;
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Direction == DIR_IN || sig->Direction == DIR_OUT)
		{
			if (first)  F0("\n");
			else        F0(",\n");
			first = false;

			const char *dt_str = sig->DataType == DATA_TYPE_WORD ? "[16:0]" : "      ";
			const char *dir_str = sig->Direction == DIR_IN ? "input " : "output";
			F3("\t%s %s %s", dir_str, dt_str, sig->Name());
		}
	}
	F0("\n);\n");


	// Instantiate local wires, connections, and delays
	GenerateVerilogWires(f);

	// Declare wires for results of non-trivial TF expressions
	// Also count the number of non-trivial expressions
	int num_nontrivial_tfs = 0;
	for (int i=0; i < num_tfs; i++)
	{
		// Skip hard-coded 0, 1, and hold
		const TruthFunction *tf = &tf_logic[i];
		const Signal *sig = tf_regs[i];
		if (!tf->IsFalse() && !tf->IsTrue() && !tf->IsHold(sig))
		{
			F1("\twire          %s$_next;\n", sig->Name());
			num_nontrivial_tfs++;
		}
	}


	// Built-in signals
	F0("\n");
	F0("\twire          status;\n");
	F0("\twire          carry;\n");
	F0("\twire          zero;\n");
	F0("\twire          negative;\n");
	F0("\twire          overflow;\n");

	// Wire up a port to view the current ALU state
	F1("\n\twire   [2:0]  instruction = %s.current_state;\n", Name());
	F0("\n");

	// Named constants that do not connect directly to the word_reg outputs
	// assign ConstVal = 17'd123;
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Behavior == BEHAVIOR_CONST && sig->DataType == DATA_TYPE_WORD)
		{
			bool word_reg = ((sig->RegisterNumber >= ALU_WORD_REG_OFFSET) && (sig->RegisterNumber < ALU_WORD_REG_OFFSET + MAX_ALU_WORD_REGS));
			if (!sig->Anonymous && !word_reg)
			{
				F2("\tassign %s = 17'd%d;\n", sig->Name(), sig->InitialValue);
			}
		}
	}

	// TFA branch logic
	// assign BranchCond0 = ~BitReg3 & BitWire2;
	char tf_str_buf[TF_STRING_BUFFER_LENGTH];
	for (int i=0; i < num_branches; i++)
	{
		const Signal *sig = branches[i];
		const TruthFunction *tf = &branch_logic[i];
		const char *tf_expr = tf->ToVerilogExpression(tf_str_buf, TF_STRING_BUFFER_LENGTH);
		F2("\tassign %s = %s;\n", sig->Name(), tf_expr);
	}


	// TF logic
	// assign Clock$_next = ~Clock;
	// Declare wires for results of non-trivial TF expressions
	if (num_nontrivial_tfs > 0)
	{
		F0("\n");
		for (int i=0; i < num_tfs; i++)
		{
			// Skip hard-coded 0, 1, and hold
			const TruthFunction *tf = &tf_logic[i];
			const Signal *sig = tf_regs[i];
			if (!tf->IsFalse() && !tf->IsTrue() && !tf->IsHold(sig))
			{
				const char *tf_expr = tf->ToVerilogExpression(tf_str_buf, TF_STRING_BUFFER_LENGTH);
				F2("\tassign %s$_next = %s;\n", sig->Name(), tf_expr);
			}
		}
	}


	// Instantiate other connections and delays
	GenerateVerilogConnections(f);
	GenerateVerilogDelays(f);


	// ALU
	F0("\n\tALU ");
	// Note: #( is generated on demand

	// Register initialization
	//      .WORD0_REG_INIT("0"),
	//      .TF0_REG_INIT("0"),
	first = true;
	for (int i=0; i < num_word_regs; i++)
	{
		if (first)  F0("#(\n");
		else        F0(",\n");
		first = false;

		const Signal *sig = word_regs[i];
		F2("\t\t.WORD%d_REG_INIT(\"%d\")", i, sig->InitialValue);
	}
	for (int i=0; i < num_tfs; i++)
	{
		if (first)  F0("#(\n");
		else        F0(",\n");
		first = false;

		const Signal *sig = tf_regs[i];
		F2("\t\t.TF%d_REG_INIT(\"%d\")", i, sig->InitialValue);
	}


	// Warm Reset Affects
	for (int i=0; i < num_word_regs; i++)
	{
		const Signal *sig = word_regs[i];
		if (sig->UsesWarmReset)
		{
			if (first)  F0("#(\n");
			else        F0(",\n");
			first = false;

			F1("\t\t.WARM_RESET_AFFECTS_WORD%d(\"TRUE\")", i);
		}
	}
	for (int i=0; i < num_tfs; i++)
	{
		const Signal *sig = tf_regs[i];
		if (sig->UsesWarmReset)
		{
			if (first)  F0("#(\n");
			else        F0(",\n");
			first = false;

			F1("\t\t.WARM_RESET_AFFECTS_TF%d_REG(\"TRUE\")", i);
		}
	}



	// Instructions
	if (num_instructions > 0)
	{
		for (int i=0; i < num_instructions; i++)
		{
			if (first)  F0("#(\n");
			else        F0(",\n\n");
			first = false;

			// Instruction
			const AluInstruction *inst = instructions[i];
			if (inst->fcn.Fcn == NULL)
			{
				// nop
				F1("\t\t.INSTR%d(\"nop\")", i);
			}
			else
			{
				F1("\t\t.INSTR%d(\"", i);
				if (inst->num_dests > 0)
				{
					for (int j=0; j < inst->num_dests; j++)
					{
						if (j > 0)
							F0(",");
						F1("%s", ToInstructionString(inst->dests[j]));
					}
				}
				else
				{
					F0("none");
				}
				F1(" = %s", inst->fcn.Fcn->Name());
				int nargs = inst->fcn.Fcn->NumArgs();
				for (int j=0; j < nargs; j++)
				{
					if (j > 0)
						F0(",");
					else
						F0(" ");

					if (inst->fcn.Fcn->ArgType(j) == 'k')
					{
						F1("%d", inst->fcn.Args[j].i);
					}
					else
					{
						Signal *arg = inst->fcn.Args[j].sig;
						F1("%s", ToInstructionString(arg));
					}
				}
				F0(";\")");
			}


			// v_out
			if (inst->v_out != 0)   // 0 is default
			{
				const char *v_out_str = "";
				switch (inst->v_out)
				{
					case 0:     v_out_str = "0";  break;
					case 1:     v_out_str = "1";  break;
					case 2:     v_out_str = "V_IN";  break;
					case 3:     v_out_str = "HOLD";  break;
				}
				F2(",\n\t\t.INSTR%d_V_OUT(\"%s\")", i, v_out_str);
			}


			// cond_bypass, cond_update_vr, cond_update_tf
			if (inst->cond_bypass)
			{
				F1(",\n\t\t.INSTR%d_COND_BYPASS(\"TRUE\")", i);
			}
			if (inst->cond_update_vr)
			{
				F1(",\n\t\t.INSTR%d_COND_UPDATE_VR(\"TRUE\")", i);
			}
			if (inst->cond_update_tf)
			{
				F1(",\n\t\t.INSTR%d_COND_UPDATE_TF(\"TRUE\")", i);
			}


			// TF Overrides
			for (int j=0; j < inst->num_tf_overrides; j++)
			{
				int tf_reg = inst->tf_overrides[j]->RegisterNumber - ALU_TF_REG_OFFSET;
				int tf_val = inst->tf_override_values[i];
				F3(",\n\t\t.INSTR%d_FORCE_TF%d(\"%d\")", i, tf_reg, tf_val);
			}

			// Latches
			if (inst->num_latches > 0)
			{
				F1(",\n\t\t.INSTR%d_LATCH(\"", i);
				for (int j=0; j < inst->num_latches; j++)
				{
					F1("%s", ToInstructionString(inst->latches[j]));
					if (j < inst->num_latches-1)
						F0(",");
				}
				F0("\")");
			}

			// wait_for_v
			if (inst->wait_for_v)
			{
				F1(",\n\t\t.INSTR%d_WAIT_V(\"TRUE\")", i);
			}


			// Branch
			if ((inst->branch_indexes[0] == inst->branch_indexes[1]) &&
				(inst->branch_indexes[0] == inst->branch_indexes[2]) &&
				(inst->branch_indexes[0] == inst->branch_indexes[3]))
			{
				// JUMP
				if (inst->branch_indexes[0] == i+1)
				{
					// Implicit jump to next instruction
				}
				else
				{
					F2(",\n\t\t.INSTR%d_JUMP(\"%d\")", i, inst->branch_indexes[0]);
				}
			}
			else if ((inst->branch_indexes[0] == inst->branch_indexes[2]) &&
					 (inst->branch_indexes[1] == inst->branch_indexes[3]))
			{
				// B0  (note, backwards from CASE)
				F3(",\n\t\t.INSTR%d_B0(\"%d,%d\")", i, inst->branch_indexes[1], inst->branch_indexes[0]);
			}
			else if ((inst->branch_indexes[0] == inst->branch_indexes[1]) &&
					 (inst->branch_indexes[2] == inst->branch_indexes[3]))
			{
				// B1  (note, backwards from CASE)
				F3(",\n\t\t.INSTR%d_B1(\"%d,%d\")", i, inst->branch_indexes[2], inst->branch_indexes[0]);
			}
			else
			{
				// CASE
				F5(",\n\t\t.INSTR%d_CASE(\"%d,%d,%d,%d\")", i, inst->branch_indexes[0], inst->branch_indexes[1], inst->branch_indexes[2], inst->branch_indexes[3]);
			}
		}
	}

	// )
	if (!first)
		F0("\n\t)\n\t");

	// ModuleName
	// (
	F1("%s\n", Name());
	F0("\t(\n");


	// word_ins
	if (num_word_ins > 0)
	{
		for (int i=0; i < num_word_ins; i++)
		{
			const Signal *sig = word_ins[i];

			if (sig->Behavior == BEHAVIOR_CONST && sig->Anonymous)
			{
				// Directly drive anonymous constants into word_in ports
				F2("\t\t.word%d_in(17'd%d),\n", i, sig->InitialValue);
			}
			else
			{
				// Connect to signal by name
				F2("\t\t.word%d_in(%s),\n", i, sig->Name());
			}
		}
		F0("\n");
	}


	// carry_ins
	if (num_carry_ins > 0)
	{
		for (int i=0; i < num_carry_ins; i++)
		{
			const Signal *sig = carry_ins[i];

			// Connect to signal by name
			F2("\t\t.carry%d_in(%s),\n", i, sig->Name());
		}
		F0("\n");
	}


	// word_regs
	if (num_word_regs > 0)
	{
		for (int i=0; i < num_word_regs; i++)
		{
			const Signal *sig = word_regs[i];

			// Connect to signal by name
			F2("\t\t.word%d_reg(%s),\n", i, sig->Name());
		}
		F0("\n");
	}


	// TFA inputs
	if (num_branches > 0 || cond_bypass || cond_update)
	{
		for (int i=0; i < num_branches; i++)
		{
			const Signal *sig = branches[i];
			F2("\t\t.tfa_branch%d_in(%s),\n", i, sig->Name());
		}
		if (cond_bypass)
		{
			F1("\t\t.tfa_cond_bypass_in(%s),\n", cond_bypass->Name());
		}
		if (cond_update)
		{
			F1("\t\t.tfa_cond_update_in(%s),\n", cond_update->Name());
		}
		F0("\n");
	}

	// tf_regs and tf_ins
	if (num_tfs > 0)
	{
		for (int i=0; i < num_tfs; i++)
		{
			const Signal *sig = tf_regs[i];

			// tf_reg
			F2("\t\t.tf%d_reg(%s),\n", i, sig->Name());


			// tf_in
			// Special cases for hard-coded 0, 1, and hold
			const TruthFunction *tf = &tf_logic[i];
			if (tf->IsFalse())
			{
				F1("\t\t.tf%d_in(1'b0),\n", i);
			}
			else if (tf->IsTrue())
			{
				F1("\t\t.tf%d_in(1'b1),\n", i);
			}
			else if (tf->IsHold(sig))
			{
				// hold simply connects tf_reg output to input
				F2("\t\t.tf%d_in(%s),\n", i, sig->Name());
			}
			else
			{
				// Use result of assignment statement above
				F2("\t\t.tf%d_in(%s$_next),\n", i, sig->Name());
			}
		}
		F0("\n");
	}


	// warm_reset_in, v_in
	const Signal *warm_reset_signal	= (const Signal *) Alu::BuiltinDefinition()->Symbols()->Get("warm_reset");
	const Signal *v_in_signal		= (const Signal *) Alu::BuiltinDefinition()->Symbols()->Get("v_in");

	first = true;
	int ncon = ConnectionCount();
	for (int i=0; i < ncon; i++)
	{
		const Connection *con = GetConnection(i);
		if (con->Destination.ResolvedSignal == warm_reset_signal)
		{
			F1("\t\t.warm_reset_in(%s),\n", con->Source.ResolvedSignal->Name());
			first = false;
		}
		if (con->Destination.ResolvedSignal == v_in_signal)
		{
			F1("\t\t.v_in(%s),\n", con->Source.ResolvedSignal->Name());
			first = false;
		}
	}
	if (!first)
	{
		F0("\n");
	}


	// Built-in signals
	F0("\t\t.status_bit(status),\n");
	F0("\t\t.carry_bit(carry),\n");
	F0("\t\t.zero_bit(zero),\n");
	F0("\t\t.sign_bit(negative),\n");
	F0("\t\t.overflow_bit(overflow)\n");       // no comma - last signal


	// );
	// endmodule
	F0("\t);\n\n");
	F0("endmodule\n");
	F0("\n");
}

