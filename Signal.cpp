
#include "Signal.h"
#include "Module.h"
#include "Common.h"

Signal::Signal(const char *name, SignalBehavior behavior, SignalDataType dataType, SignalDirection direction, int initialValue, bool anonymous, bool automatic)
	: Symbol(name), Behavior(behavior), DataType(dataType), Direction(direction), InitialValue(initialValue), RegisterNumber(-1), UsesWarmReset(0), Anonymous(anonymous), Automatic(automatic), BitSliceIndex(-1), DelayCount(0), BaseSignal(NULL), module(NULL)
{
}

Signal::~Signal()
{
	module = NULL;
}

bool Signal::Shadowable() const
{
	// Allow most signals to be shadowable, but deny shadowing for builtins like 'status'
	return (Behavior != BEHAVIOR_BUILTIN);
}

void Signal::Print(FILE *f) const
{
	const char *anonymous_str = Anonymous ? "anonymous " : "";
	const char *automatic_str = Automatic ? "automatic " : "";

	const char *dir_str = "";
	if (Direction == DIR_IN)
		dir_str = "input ";
	else if (Direction == DIR_OUT)
		dir_str = "output ";

	const char *behavior_str = "";
	if (Behavior == BEHAVIOR_WIRE)
		behavior_str = "wire";
	else if (Behavior == BEHAVIOR_REG)
		behavior_str = "reg";
	else if (Behavior == BEHAVIOR_CONST)
		behavior_str = "const";
	else if (Behavior == BEHAVIOR_BRANCH)
		behavior_str = "branch";
	else if (Behavior == BEHAVIOR_COND_BYPASS)
		behavior_str = "cond_bypass";
	else if (Behavior == BEHAVIOR_COND_UPDATE)
		behavior_str = "cond_update";
	else if (Behavior == BEHAVIOR_DELAY)
		behavior_str = "delay";
	else if (Behavior == BEHAVIOR_BIT_SLICE)
		behavior_str = "bit_slice";

	const char *data_type_str = "";
	if (DataType == DATA_TYPE_BIT)
		data_type_str = "bit";
	else if (DataType == DATA_TYPE_WORD)
		data_type_str = "word";

	char delay_str[32];
	delay_str[0] = 0;
	if (Behavior == BEHAVIOR_DELAY)
		sprintf(delay_str, " D%d", DelayCount);

	char bit_slice_str[32];
	bit_slice_str[0] = 0;
	if (Behavior == BEHAVIOR_BIT_SLICE)
		sprintf(delay_str, " B%d", BitSliceIndex);

	char regnum_str[32];
	regnum_str[0] = 0;
	if (RegisterNumber >= 0)
		sprintf(regnum_str, " %d", RegisterNumber);

	const char *reset_str = UsesWarmReset ? " R" : "";

	char value_str[32];
	value_str[0] = 0;
	if (Behavior == BEHAVIOR_CONST || Behavior == BEHAVIOR_REG)
	{
		if (InitialValue < 0)
			sprintf(value_str, " = (unknown)");
		else
			sprintf(value_str, " = 0x%04x", InitialValue);
	}

	fprintf(f, "%s : %s%s%s%s %s%s%s%s%s%s", name, anonymous_str, automatic_str, dir_str, behavior_str, data_type_str, delay_str, bit_slice_str, regnum_str, reset_str, value_str);
}


// Create new signals based on the original
// These methods may lookup in the current module for identical signals, and return those,
// or they may create new automatic signals, which are added to the current module

// Return a signal delayed by the given number of clocks
Signal *Signal::Delay(int delay) const
{
	if (delay < 1 || delay > MAX_SIGNAL_DELAY)
	{
		yyerrorf("Delay must be a value from 1 to %d", MAX_SIGNAL_DELAY);
		return NULL;
	}

	else if (Behavior == BEHAVIOR_BIT_SLICE)
	{
		// Delaying a bit-slice or v-bit should return a bit-slice of the delayed signal
		// (auto-commutation of delay and bit-slice)
		Signal *delayedBase = BaseSignal->Delay(delay);
		if (BitSliceIndex == V_BIT_SLICE_INDEX)
			return delayedBase->VBit();
		else
			return delayedBase->BitSlice(BitSliceIndex);
	}

	else if (Behavior == BEHAVIOR_DELAY)
	{
		// Delaying a delayed signal has the same effect as delaying the original signal by the sum of the two delays
		// This is done here to prevent recursive delays, so the BaseSignal will always point to an undelayed signal
		return BaseSignal->Delay(DelayCount + delay);
	}

	else if (Behavior == BEHAVIOR_BUILTIN && Direction == DIR_NONE)
	{
		// It is illegal to delay built-in signals that are not available for connection as inputs or outputs
		yyerrorf("Cannot delay built-in signal '%s'", Name());
		return NULL;
	}

	// Synthesize a name for the delayed signal as in:  original$2
	char *tmpName = (char *) malloc(strlen(Name()) + 32);
	sprintf(tmpName, "%s$%d", Name(), delay);

	// Check if the delayed signal already exists in the same module as the original signal
	Signal *exists = module->GetSignal(tmpName);
	if (exists)
	{
		free((void *) tmpName);
		return exists;
	}

	// Create a new signal with the same DataType and no Direction, which points to this signal with the specified delay
	const char *newName = strings->AddString(tmpName);
	free((void *) tmpName);

	Signal *result = new Signal(newName, BEHAVIOR_DELAY, DataType, DIR_NONE);
	result->BaseSignal = (Signal *) this;
	result->DelayCount = delay;
	result->Automatic = true;
	result->Location = CurrentLocation();

	// Add the new signal to the same module as this signal
	if (!module->AddSignal(result))
	{
		// We should never get here, because we already checked for the name
		// Somehow if another symbol like a module gets the same name, it could fail.
		// However, the name-mangling rules for delayed signals should prevent this.
		yyerrorf("Could not add delayed signal: '%s'", result->Name());
		return NULL;
	}

	return result;
}

// Return a bit-sliced signal that refers to the v-bit of a base word
Signal *Signal::VBit() const
{
	if (DataType != DATA_TYPE_WORD)
	{
		yyerrorf("Illegal reference to v-bit.  Signal '%s' is not declared as a word", Name());
		return NULL;
	}

	else if (Behavior == BEHAVIOR_CONST)
	{
		yyerrorf("Cannot refer to the v-bit of a constant: '%s'", Name());
		return NULL;
	}

	else if (Behavior == BEHAVIOR_BUILTIN && Direction == DIR_NONE)
	{
		// It is illegal to reference the v-bit of built-in signals that are not available for connection as inputs or outputs
		yyerrorf("Illegal reference to v-bit of built-in signal '%s'", Name());
		return NULL;
	}

	// Synthesize an anonymous name for the bit-sliced signal as in:  .original[16]
	char *tmpName = (char *) malloc(strlen(Name()) + 32);
	sprintf(tmpName, "%s[%d]", Name(), V_BIT_SLICE_INDEX);

	// Check if the bit-sliced signal already exists in the same module as the original signal
	Signal *exists = module->GetSignal(tmpName);
	if (exists)
	{
		free((void *) tmpName);
		return exists;
	}

	// Create a new anonymous bit signal with no Direction, which points to this signal and refers to the v-bit slice index
	const char *newName = strings->AddString(tmpName);
	free((void *) tmpName);

	Signal *result = new Signal(newName, BEHAVIOR_BIT_SLICE, DATA_TYPE_BIT, DIR_NONE);
	result->BaseSignal = (Signal *) this;
	result->BitSliceIndex = V_BIT_SLICE_INDEX;
	result->Anonymous = true;
	result->Automatic = true;
	result->Location = CurrentLocation();

	// Add the new signal to the same module as this signal
	if (!module->AddSignal(result))
	{
		// We should never get here, because we already checked for the name
		// Somehow if another symbol like a module gets the same name, it could fail.
		// However, the name-mangling rules for delayed signals should prevent this.
		yyerrorf("Could not add v-bit signal: '%s'", result->Name());
		return NULL;
	}

	return result;
}

// Return a bit-sliced signal that refers to one of the first 4 bits of a word reg
// Only a local word reg may have a bit-slice taken, as the hardware only supports references
// to bits 0-3 of a nearest neighbor register in a TF or TFA expression
Signal *Signal::BitSlice(int index) const
{
	if (DataType != DATA_TYPE_WORD || Behavior != BEHAVIOR_REG || index < 0 || index > MAX_REG_BIT_SLICE_INDEX)
	{
		yyerrorf("Illegal bit-slice of signal '%s'.  Can only use bits 0 to %d of a local word reg", Name(), MAX_REG_BIT_SLICE_INDEX);
		return NULL;
	}

	// Synthesize an anonymous name for the bit-sliced signal as in:  .original[3]
	char *tmpName = (char *) malloc(strlen(Name()) + 32);
	sprintf(tmpName, "%s[%d]", Name(), index);

	// Check if the bit-sliced signal already exists in the same module as the original signal
	Signal *exists = module->GetSignal(tmpName);
	if (exists)
	{
		free((void *) tmpName);
		return exists;
	}

	// Create a new anonymous bit signal with no Direction, which points to this signal and refers to the v-bit slice index
	const char *newName = strings->AddString(tmpName);
	free((void *) tmpName);

	Signal *result = new Signal(newName, BEHAVIOR_BIT_SLICE, DATA_TYPE_BIT, DIR_NONE);
	result->BaseSignal = (Signal *) this;
	result->BitSliceIndex = index;
	result->Anonymous = true;
	result->Automatic = true;
	result->Location = CurrentLocation();

	// Add the new signal to the same module as this signal
	if (!module->AddSignal(result))
	{
		// We should never get here, because we already checked for the name
		// Somehow if another symbol like a module gets the same name, it could fail.
		// However, the name-mangling rules for delayed signals should prevent this.
		yyerrorf("Could not add bit-sliced signal: '%s'", result->Name());
		return NULL;
	}

	return result;
}



// Initialize a SignalReference.  This type cannot use a constructor,
// because it is used in a union as the yylval in the parser
void SignalReference::Init()
{
	Id.Init();
	DelayCount = 0;
	VBit = false;
	Direction = DIR_NONE;
	ResolvedSignal = NULL;
	ResolvedInstance = NULL;
}

// Deletes the underlying DottedIdentifier
void SignalReference::Delete()
{
	Id.Delete();
	DelayCount = 0;
	VBit = false;
	Direction = DIR_NONE;
	ResolvedSignal = NULL;
	ResolvedInstance = NULL;
}


// Determines whether two SignalReferences refer to the same signal
bool SignalReference::operator==(const SignalReference &ref) const
{
	if (Id != ref.Id) return false;
	if (DelayCount != ref.DelayCount) return false;
	if (VBit != ref.VBit) return false;
	if (Direction != ref.Direction) return false;

	// If both are resolved, then they must match, otherwise skip this check
	if (ResolvedSignal && ref.ResolvedSignal)
	{
		if (ResolvedSignal != ref.ResolvedSignal) return false;
		if (ResolvedInstance != ref.ResolvedInstance) return false;
	}

	return true;
}

bool SignalReference::operator!=(const SignalReference &ref) const
{
	// Invert result of operator==
	return !(*this == ref);
}

// Simple factory method from a DottedIdentifier, in lieu of a constructor
SignalReference SignalReference::FromId(const DottedIdentifier &id, SignalDirection direction)
{
	SignalReference result;
	result.Init();
	result.Id = id;
	result.Direction = direction;
	return result;
}




// Initialize a SignalReferenceList.  This type cannot use a constructor,
// because it is used in a union as the yylval in the parser
void SignalReferenceList::Init()
{
	Ids.Init();
	DelayCount = 0;
	Direction = DIR_NONE;
}

// Deletes the underlying DottedIdentifierList
void SignalReferenceList::Delete()
{
	Ids.Delete();
	DelayCount = 0;
	Direction = DIR_NONE;
}

// Simple factory method from a single DottedIdentifier, in lieu of a constructor
SignalReferenceList SignalReferenceList::FromId(const DottedIdentifier &id, SignalDirection direction)
{
	SignalReferenceList result;
	result.Init();
	result.Ids.Id = id;
	result.Direction = direction;
	return result;
}

// Simple factory method from a DottedIdentifierList, in lieu of a constructor
SignalReferenceList SignalReferenceList::FromIdList(const DottedIdentifierList &idlist, SignalDirection direction)
{
	SignalReferenceList result;
	result.Init();
	result.Ids = idlist;
	result.Direction = direction;
	return result;
}
