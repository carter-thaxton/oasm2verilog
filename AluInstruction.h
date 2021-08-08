
#ifndef ALU_INSTRUCTION_H
#define ALU_INSTRUCTION_H

#include "Signal.h"
#include "AluFunctionCall.h"
#include "Alu.h"
#include "TF.h"
#include <vector>

// Forward declaration for friend status
class Alu;

class AluInstruction
{
public:
	// Initializes instruction with all properties unset
	AluInstruction(const char *label = NULL);
	virtual ~AluInstruction();

	// Get Label
	const char *Label() const;

	// Set AluFunction, returns NULL if error
	AluFunctionCall *SetFunction(const AluFunctionCall &fcn);

	// Add destinations, returns NULL if error
	Signal *AddDestination(Signal *dest);

	// Add TF override, returns NULL if error
	Signal *AddTFOverride(Signal *dest, int value);

	// Add latch of word_in, returns NULL if error
	Signal *AddLatch(Signal *word_in);

	// Set flags, returns false if error
	bool SetCondBypass();
	bool SetCondUpdateVR();
	bool SetCondUpdateTF();
	bool SetWaitForV();
	bool SetVOut(int value);	// 0, 1, (2 = v_in)

	// Set branches, returns false if error
	bool Goto(const char *inst);
	bool If(Signal *cond, const char *inst_if);
	bool IfNot(Signal *cond, const char *inst_if);
	bool IfElse(Signal *cond, const char *inst_if, const char *inst_else);
	bool Case(Signal *cond1, Signal *cond0, const char *inst0, const char *inst1, const char *inst2, const char *inst3);

	// Apply defaults to instructions, after setting all other properties
	void ApplyDefaults();

	// Print instruction for debug
	void Print(FILE *f) const;

	SourceCodeLocation Location;

private:
	bool BranchAlreadySet() const;

	const char *label;
	AluFunctionCall fcn;

	Signal *dests[MAX_ALU_WORD_REGS];
	int num_dests;

	Signal *tf_overrides[MAX_TFS];
	int tf_override_values[MAX_TFS];
	int num_tf_overrides;

	Signal *branch_conds[2];
	const char *branch_labels[4];
	int branch_indexes[4];

	Signal *latches[MAX_ALU_WORD_INS];
	int num_latches;

	int cond_bypass;
	int cond_update_vr;
	int cond_update_tf;
	int wait_for_v;
	int v_out;

	int index;

	// Alu class directly manipulates instructions after parsing
	friend class Alu;
};


#endif
