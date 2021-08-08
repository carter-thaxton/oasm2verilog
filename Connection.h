
#ifndef CONNECTION_H
#define CONNECTION_H

#include "Signal.h"
#include "Common.h"

class Module;
class Instance;

class Connection
{
public:
	Connection();
	Connection(Module *module, const SignalReference &source, const SignalReference &dest, const SourceCodeLocation &location = CurrentLocation());
	Connection(Module *module, Signal *sourceSignal, Instance *signalInstance, Signal *destSignal, Instance *destInstance, const SourceCodeLocation &location = CurrentLocation());
	virtual ~Connection();

	void Print(FILE *f) const;

	SignalReference Source;
	SignalReference Destination;
	SourceCodeLocation Location;
	Module *module;

	bool operator==(const Connection &connection) const;
	bool operator!=(const Connection &connection) const;

	// Resolves a connection, in the context of its module.
	// Calls Flatten afer being resolved, which may modify this and parent modules
	bool Resolve();

	// Modifies its module and surrounding ones to create intermediate signals,
	// because Verilog does not support direct connections between ports,
	// or to add outer connections when inner module syntax is used to
	// refer to a signal at a higher scope
	bool Flatten();

private:

	// Look up signal in local scope first, and work upwards to top-level module definition,
	// following dotted identifiers to work back downwards into instances.
	// The sigref provided to these methods are modified to resolve the instance and signal
	bool LookUpForSignal(const Module *module, const DottedIdentifier &id, SignalReference &sigref) const;
	bool LookDownForSignal(const Module *module, const DottedIdentifier &id, SignalReference &sigref) const;
};


// Unresolved connection to a higher-level signal, to be flatted at each level up the
// inner module hierarchy, during ResolveConnections phase.
struct OuterConnection
{
	OuterConnection();
	OuterConnection(Module *module, SignalDirection direction, Signal *sourceSignal, Instance *sourceInstance, Signal *destinationSignal, Instance *destinationInstance);

	Module *module;
	SignalDirection Direction;

	Signal *SourceSignal;
	Instance *SourceInstance;
	Signal *DestinationSignal;
	Instance *DestinationInstance;

	void Print(FILE *f) const;

	bool operator==(const OuterConnection &outerConnection) const;
	bool operator!=(const OuterConnection &outerConnection) const;
};

#endif
