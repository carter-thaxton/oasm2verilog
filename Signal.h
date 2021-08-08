
#ifndef SIGNAL_H
#define SIGNAL_H

#include "Symbol.h"
#include "Expression.h"
#include "Identifier.h"
#include "Instance.h"
#include "Common.h"

// COAST has a built-in limit for the delay on a party-line
#define MAX_SIGNAL_DELAY		(30)

// Bit 16 of a word is always the v-bit
#define V_BIT_SLICE_INDEX		(16)

// Only bits 0-3 are allowed as register bit-slices
#define MAX_REG_BIT_SLICE_INDEX	(3)


class Module;

enum SignalBehavior
{
	BEHAVIOR_UNKNOWN,
	BEHAVIOR_WIRE,
	BEHAVIOR_REG,
	BEHAVIOR_CONST,
	BEHAVIOR_BUILTIN,
	BEHAVIOR_BRANCH,
	BEHAVIOR_COND_UPDATE,
	BEHAVIOR_COND_BYPASS,
	BEHAVIOR_BIT_SLICE,
	BEHAVIOR_DELAY,
};

enum SignalDataType
{
	DATA_TYPE_UNKNOWN,
	DATA_TYPE_BIT,
	DATA_TYPE_WORD,
};

enum SignalDirection
{
	DIR_NONE,
	DIR_IN,
	DIR_OUT,
};

class Signal : public Symbol
{
public:
	Signal(const char *name, SignalBehavior behavior = BEHAVIOR_WIRE, SignalDataType dataType = DATA_TYPE_WORD, SignalDirection direction = DIR_NONE, int initialValue = -1, bool anonymous = false, bool generated = false);
	virtual ~Signal();

	// Overridden from Symbol
	virtual SymbolTypeId SymbolType() const { return SYMBOL_SIGNAL; }
	virtual void Print(FILE *f) const;
	virtual bool Shadowable() const;

	// Create new signals based on the original
	// These methods may lookup in the current module for identical signals, and return those,
	// or they may create new anonymous signals, which are added to the current module
	Signal *Delay(int delay) const;
	Signal *VBit() const;
	Signal *BitSlice(int index) const;

	// Public members
	SignalBehavior Behavior;
	SignalDataType DataType;
	SignalDirection Direction;
	int InitialValue;					// InitialValue is -1 by default, which means uninitialized.  0 or 0xFFFF (-1 truncated to 16 bits) are explicit values.
	int RegisterNumber;					// Some signals are assigned to a register location, -1 if not initialized.
	bool UsesWarmReset;					// Dynamically initialized by warm_reset.
	bool Anonymous;						// Bit-slices and some constants are created as anonymous.
	bool Automatic;						// Outer connections and direct connections between ports use automatic signals with mangled names.

	// These members are used for complex signals that reference another BaseSignal  (bit-slicing and delays)
	int BitSliceIndex;                  // -1 if no bit-slicing.  BitSliceIndex of 16 indicates use of the v-bit
	int DelayCount;                     // When BEHAVIOR_DELAY, this delay indicates the number of clock delays.  Otherwise it is 0.
	Signal *BaseSignal;                 // Bit-sliced and delayed signals reference another signal as their source

	SourceCodeLocation Location;

	// Points to the containing module
	Module *module;
};


// Type used to carry around a signal reference until the ResolveConnections phase
struct SignalReference
{
	DottedIdentifier Id;		// Signal reference by dotted name, which may refer to an instance or a module, and ends in a signal name
	int DelayCount;				// Number of clock delays to synthesize after the referenced signal
	bool VBit;					// Whether the v-bit is referenced by this  (note: other bit-slices are not supported at the structural level)
	SignalDirection Direction;	// Indicates whether the signal is an input, output, or local signal, which will be used during signal resolution

	// Set once associated with a signal during the ResolveConnections phase
	Signal *ResolvedSignal;
	Instance *ResolvedInstance;

	void Init();				// Cannot have a constructor or destructor, because this is used in the yylval union 
	void Delete();				// Deletes the underlying DottedIdentifier

	// In lieu of a constructor, this factory method creates a SignalReference from an identifier
	static SignalReference FromId(const DottedIdentifier &id, SignalDirection direction = DIR_NONE);

	bool operator==(const SignalReference &ref) const;
	bool operator!=(const SignalReference &ref) const;
};

// Type used to carry a list of signals through a right_signal_expr
struct SignalReferenceList
{
	DottedIdentifierList Ids;
	int DelayCount;
	SignalDirection Direction;

	void Init();				// Cannot have a constructor or destructor, because this is used in the yylval union 
	void Delete();				// Deletes the underlying DottedIdentifierList

	static SignalReferenceList FromId(const DottedIdentifier &id, SignalDirection direction = DIR_NONE);
	static SignalReferenceList FromIdList(const DottedIdentifierList &idlist, SignalDirection direction = DIR_NONE);
};


#endif
