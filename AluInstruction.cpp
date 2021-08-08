
#include "AluInstruction.h"
#include "AluFunction.h"


// Initializes instruction with all properties unset
AluInstruction::AluInstruction(const char *label)
	: label(label), dests(), num_dests(0),
	  tf_overrides(), tf_override_values(), num_tf_overrides(0),
	  branch_conds(), branch_labels(), branch_indexes(),
	  latches(), num_latches(0),
	  cond_bypass(-1), cond_update_vr(-1), cond_update_tf(-1), wait_for_v(-1), v_out(-1),
	  index(-1)
{
	fcn.Fcn = NULL;
}

AluInstruction::~AluInstruction()
{
}

// Get Label
const char *AluInstruction::Label() const
{
	return label;
}

// Set AluFunction, returns NULL if error
AluFunctionCall *AluInstruction::SetFunction(const AluFunctionCall &fcn)
{
	if (this->fcn.Fcn)
		return NULL;

	this->fcn = fcn;
	return &(this->fcn);
}

// Add destinations, returns NULL if error
Signal *AluInstruction::AddDestination(Signal *dest)
{
	// Check if too many destinations
	if (num_dests >= MAX_ALU_WORD_REGS)
		return NULL;

	// Check for repeated destinations
	for (int i=0; i < num_dests; i++)
	{
		if (dests[i] == dest)
		{
			return NULL;
		}
	}
	
	// Add as destination
	dests[num_dests] = dest;
	num_dests++;

	return dest;
}

// Add TF override, returns NULL if error
Signal *AluInstruction::AddTFOverride(Signal *dest, int value)
{
	// Check for too many overrides
	if (num_tf_overrides >= MAX_TFS)
		return NULL;

	// Check range
	if (value < 0 || value > 1)
		return NULL;

	// Destination must be valid
	if (dest == NULL)
		return NULL;

	// Check for duplicate overrides of same TF
	for (int i=0; i < num_tf_overrides; i++)
	{
		if (tf_overrides[i] == dest)
			return NULL;
	}

	// Add TF override and return dest
	tf_overrides[num_tf_overrides] = dest;
	tf_override_values[num_tf_overrides] = value;
	num_tf_overrides++;

	return dest;
}

// Add latch of word_in, returns NULL if error
Signal *AluInstruction::AddLatch(Signal *word_in)
{
	// Check for too many latches
	if (num_latches >= MAX_ALU_WORD_INS)
		return NULL;

	// Signal must be valid
	if (word_in == NULL)
		return NULL;

	// Check for duplicate latches of same signal
	for (int i=0; i < num_latches; i++)
	{
		if (latches[i] == word_in)
			return NULL;
	}

	// Add latch and return word_in
	latches[num_latches] = word_in;
	num_latches++;

	return word_in;
}


// Set flags, returns false if error
bool AluInstruction::SetCondBypass()
{
	if (cond_bypass < 0)
	{
		cond_bypass = 1;
		return true;
	}
	return false;
}

bool AluInstruction::SetCondUpdateVR()
{
	if (cond_update_vr < 0)
	{
		cond_update_vr = 1;
		return true;
	}
	return false;
}

bool AluInstruction::SetCondUpdateTF()
{
	if (cond_update_tf < 0)
	{
		cond_update_tf = 1;
		return true;
	}
	return false;
}

bool AluInstruction::SetWaitForV()
{
	if (wait_for_v < 0)
	{
		wait_for_v = 1;
		return true;
	}
	return false;
}

// 0, 1, (2 = v_in), (3 = hold)
bool AluInstruction::SetVOut(int value)
{
	if (v_out < 0 && value >= 0 && value <= 3)
	{
		v_out = value;
		return true;
	}
	return false;
}


// Set branches, returns false if error
bool AluInstruction::Goto(const char *inst)
{
	if (BranchAlreadySet()) return false;

	branch_labels[0] = branch_labels[1] = branch_labels[2] = branch_labels[3] = inst;

	return true;
}

bool AluInstruction::If(Signal *cond, const char *inst_if)
{
	if (BranchAlreadySet()) return false;

	branch_conds[0] = cond;
	branch_labels[0] = branch_labels[2] = NULL;
	branch_labels[1] = branch_labels[3] = inst_if;

	return true;
}

bool AluInstruction::IfNot(Signal *cond, const char *inst_if_not)
{
	if (BranchAlreadySet()) return false;

	branch_conds[0] = cond;
	branch_labels[0] = branch_labels[2] = inst_if_not;
	branch_labels[1] = branch_labels[3] = NULL;

	return true;
}

bool AluInstruction::IfElse(Signal *cond, const char *inst_if, const char *inst_else)
{
	if (BranchAlreadySet()) return false;

	branch_conds[0] = cond;
	branch_labels[0] = branch_labels[2] = inst_else;
	branch_labels[1] = branch_labels[3] = inst_if;

	return true;
}

bool AluInstruction::Case(Signal *cond1, Signal *cond0, const char *inst0, const char *inst1, const char *inst2, const char *inst3)
{
	if (BranchAlreadySet()) return false;

	branch_conds[0] = cond0;
	branch_conds[1] = cond1;
	branch_labels[0] = inst0;
	branch_labels[1] = inst1;
	branch_labels[2] = inst2;
	branch_labels[3] = inst3;

	return true;
}

bool AluInstruction::BranchAlreadySet() const
{
	return (branch_labels[0] || branch_labels[1] || branch_labels[2] || branch_labels[3] || branch_conds[0] || branch_conds[1]);
}

// Apply defaults to instructions, after setting all other properties
void AluInstruction::ApplyDefaults()
{
	if (cond_bypass < 0)
		cond_bypass = 0;

	if (cond_update_vr < 0)
		cond_update_vr = 0;

	if (cond_update_tf < 0)
		cond_update_tf = 0;

	if (wait_for_v < 0)
		wait_for_v = 0;

	if (v_out < 0)
		v_out = 0;
}

// Print for debug
void AluInstruction::Print(FILE *f) const
{
	// index and label
	const char *label_str = "";
	if (label)
		label_str = label;

	fprintf(f, "%d: '%s' ", index, label_str);


	// destinations
	if (num_dests > 0)
	{
		for (int i=0; i < num_dests; i++)
		{
			fprintf(f, "%s", dests[i]->Name());
			if (i < num_dests-1)
				fprintf(f, ", ");
		}
		fprintf(f, " = ");
	}


	// function and arguments
	const char *fcn_str = "nop";
	int nargs = 0;
	if (fcn.Fcn)
	{
		fcn_str = fcn.Fcn->Name();
		nargs = fcn.Fcn->NumArgs();
	}

	fprintf(f, "%s(", fcn_str);
	for (int i=0; i < nargs; i++)
	{
		if (fcn.Fcn->ArgType(i) == 'k')
			fprintf(f, "%d", fcn.Args[i].i);
		else
			fprintf(f, "%s", fcn.Args[i].sig->Name());

		if (i < nargs-1)
			fprintf(f, ", ");
	}
	fprintf(f, ")");

	// TF overrides
	if (num_tf_overrides > 0)
	{
		fprintf(f, "  ");
		for (int i=0; i < num_tf_overrides; i++)
		{
			fprintf(f, "%s=%d", tf_overrides[i]->Name(), tf_override_values[i]);

			if (i < num_tf_overrides-1)
				fprintf(f, ", ");
		}
	}

	// flags
	const char *v_str = "";
	switch (v_out)
	{
		case 0: v_str = "0";     break;
		case 1: v_str = "1";     break;
		case 2: v_str = "v_in";  break;
		case 3: v_str = "hold";  break;
	}

	fprintf(f, "  cb:%d  cu_vr:%d  cu_tf:%d  wait_v:%d  v:%s", cond_bypass, cond_update_vr, cond_update_tf, wait_for_v, v_str);

	// branches
	fprintf(f, "  %d,%d,%d,%d", branch_indexes[0], branch_indexes[1], branch_indexes[2], branch_indexes[3]);

	fprintf(f, "\n");
}
