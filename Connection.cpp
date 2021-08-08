
#include "Connection.h"
#include "Module.h"
#include "Instance.h"


Connection::Connection()
	: Location(CurrentLocation()), module(NULL)
{
	Source.Init();
	Destination.Init();
}

Connection::Connection(Module *module, const SignalReference &source, const SignalReference &dest, const SourceCodeLocation &location)
	: Source(source), Destination(dest), Location(location), module(module)
{
	if (dest.DelayCount != 0)
	{
		yyfatalf("Cannot connect to a delayed signal: ");
		dest.Id.Print(stderr);
	}
}

Connection::Connection(Module *module, Signal *sourceSignal, Instance *sourceInstance, Signal *destSignal, Instance *destInstance, const SourceCodeLocation &location)
	: Location(location), module(module)
{
	Source.Init();
	Destination.Init();

	Source.ResolvedSignal = sourceSignal;
	Source.ResolvedInstance = sourceInstance;
	Destination.ResolvedSignal = destSignal;
	Destination.ResolvedInstance = destInstance;
}

Connection::~Connection()
{
}

void Connection::Print(FILE *f) const
{
	// Prints in one of two forms, depending on whther the connection is resolved or not
	
	// When not yet resolved, prints as follows:
	// source -> destination
	// input source.v -> delay(2) -> output destination

	// Once resolved, prints as follows:
	// source[16] -> delay(2) -> destination

	if (Source.ResolvedSignal)
	{
		if (Source.ResolvedInstance)
		{
			if (Source.ResolvedInstance->module)
			{
				if (Source.ResolvedInstance->module != module)
					fprintf(f, "%s:", Source.ResolvedInstance->module->Name());
			}
			fprintf(f, "%s.", Source.ResolvedInstance->Name());
		}
		else
		{
			if (Source.ResolvedSignal->Behavior == BEHAVIOR_BUILTIN)
			{
				fprintf(f, "BUILTIN:");
			}
			else if (Source.ResolvedSignal->module)
			{
				if (Source.ResolvedSignal->module != module)
					fprintf(f, "%s:", Source.ResolvedSignal->module->Name());
			}
		}

		fprintf(f, "%s", Source.ResolvedSignal->Name());
	}
	else
	{
		if (Source.Direction == DIR_IN)
			fprintf(f, "input ");
		else if (Source.Direction == DIR_OUT)
			fprintf(f, "output ");

		Source.Id.Print(f);
	}

	if (Source.VBit)
		fprintf(f, ".v");

	if (Source.DelayCount > 0)
		fprintf(f, " -> delay(%d)", Source.DelayCount);

	fprintf(f, " -> ");

	if (Destination.ResolvedSignal)
	{
		if (Destination.ResolvedInstance)
		{
			if (Destination.ResolvedInstance->module)
			{
				if (Destination.ResolvedInstance->module != module)
					fprintf(f, "%s:", Destination.ResolvedInstance->module->Name());
			}
			fprintf(f, "%s.", Destination.ResolvedInstance->Name());
		}
		else
		{
			if (Destination.ResolvedSignal->Behavior == BEHAVIOR_BUILTIN)
			{
				fprintf(f, "BUILTIN:");
			}
			else if (Destination.ResolvedSignal->module)
			{
				if (Destination.ResolvedSignal->module != module)
					fprintf(f, "%s:", Destination.ResolvedSignal->module->Name());
			}
		}

		fprintf(f, "%s", Destination.ResolvedSignal->Name());
	}
	else
	{
		if (Destination.Direction == DIR_IN)
			fprintf(f, "input ");
		else if (Destination.Direction == DIR_OUT)
			fprintf(f, "output ");

		Destination.Id.Print(f);
	}
}


// Two connections are equal if their sources and destinations are equal
bool Connection::operator==(const Connection &connection) const
{
	// The modules must both be the same
	if (module && connection.module)
	{
		if (module != connection.module)
			return false;
	}

	// Note, SignalReference also implements operator==
	return (Source == connection.Source) && (Destination == connection.Destination);
}

bool Connection::operator!=(const Connection &connection) const
{
	// Invert result of operator==
	return !(*this == connection);
}




// Resolve performs a number of steps, after all modules have been parsed:
//
// - Finds source and destination signals by name, walking up the hierarchy to find the first name in the dotted identifer,
// and back down the instance tree through each name in the dotted identifer, resolving in a signal.
//
// - The resolved signal and instance are set:
//   - The signal is a reference to the signal in its module definition
//   - When the connection is to or from a port, the resolved instance is set
//   - When the connection is to or from a local signal, the resolved instance is left NULL
//
// - Check the data types of the connection, bits to bits or words to words,
//   and v-bit is only used with a word
//
// - The connection is then flattened, creating additional wires, ports,
//   and connections until each connection meets one of the following criteria:
//   - From a signal to another signal in the same module
//   - From an output port to a signal in the same module as the instance
//   - From a signal to an input port of an instance in the same module as the wire
//
bool Connection::Resolve()
{
	// Resolve only an unresolved source signal
	if (Source.ResolvedSignal == NULL)
	{
		if (Destination.Direction == DIR_IN)
		{
			// The source of a connection to an input is resolved in the parent module
			Module *parent = module->ParentModule();
			if (parent)
			{
				LookUpForSignal(parent, Source.Id, Source);
			}
			else
			{
				char buf[256];
				yyerrorfl(Location, "Illegal inward connection in top-level module '%s' from signal '%s'", module->Name(), Source.Id.ToString(buf, sizeof(buf)) );
			}
		}
		else
		{
			LookUpForSignal(module, Source.Id, Source);
		}
	}


	// Resolve only an unresolved destination signal
	if (Destination.ResolvedSignal == NULL)
	{
		if (Source.Direction == DIR_OUT)
		{
			// The destination of a connection from an output is resolved in the parent module
			Module *parent = module->ParentModule();
			if (parent)
			{
				LookUpForSignal(parent, Destination.Id, Destination);
			}
			else
			{
				char buf[256];
				yyerrorfl(Location, "Illegal outward connection in top-level module '%s' to signal '%s'", module->Name(), Destination.Id.ToString(buf, sizeof(buf)));
			}
		}
		else
		{
			LookUpForSignal(module, Destination.Id, Destination);
		}
	}


	// Only continue if both source and destination are resolved
	if (Source.ResolvedSignal && Destination.ResolvedSignal)
	{
		// Check data type of connections
		SignalDataType sourceType = Source.ResolvedSignal->DataType;
		SignalDataType destType = Destination.ResolvedSignal->DataType;

		// Check for V-bit reference of non-word signal
		if (Source.VBit)
		{
			if (sourceType != DATA_TYPE_WORD)
			{
				yyerrorfl(Location, "Illegal reference to v-bit of non-word signal '%s'", Source.ResolvedSignal->Name());
				return false;
			}
			sourceType = DATA_TYPE_BIT;
		}

		// Check for data type mismatch
		if (sourceType != destType)
		{
			const char *sourceTypeStr = "(unknown)";
			if (sourceType == DATA_TYPE_BIT)
				sourceTypeStr = "bit";
			else if (sourceType == DATA_TYPE_WORD)
				sourceTypeStr = "word";

			const char *destTypeStr = "(unknown)";
			if (destType == DATA_TYPE_BIT)
				destTypeStr = "bit";
			else if (destType == DATA_TYPE_WORD)
				destTypeStr = "word";

			yyerrorfl(Location, "Illegal connection from %s to %s", sourceTypeStr, destTypeStr);
			return false;
		}

		// Flatten by creating intermediate wires and connections
		Flatten();
		return true;
	}

	return false;
}


// Look up signal in local scope first, and work upwards to top-level module definition,
// then follow dotted identifiers back downwards into instances
// Fills in the given SignalReference with the ResolvedSignal and ResolvedInstance
bool Connection::LookUpForSignal(const Module *module, const DottedIdentifier &id, SignalReference &sigref) const
{
	const char *firstId = id.Name;

	// Look upwards through module scope for the first symbol that matches the first identifier in the dotted identifier
	while (module)
	{
		// If the first identifier refers to the module name, this acts as a scope resolution operator, and not an instance
		if (strcmp(firstId, module->Name()) == 0)
		{
			if (id.Next == NULL)
			{
				yyerrorfl(Location, "Illegal reference to a module.  Connections must refer to signals");
				sigref.ResolvedSignal = NULL;
				sigref.ResolvedInstance = NULL;
				return false;
			}
			// Look downward for signal, but do not set the instance, because it is not yet found
			return LookDownForSignal(module, *id.Next, sigref);
		}

		// Check for a signal with the corresponding name
		Signal *sig = module->GetSignal(firstId);
		if (sig)
		{
			// If a signal is found, then the dotted identifier must be complete
			// Currently, there is no signal nesting supprted (i.e. structs or bundles)
			if (id.Next)
			{
				char buf[256];
				yyerrorfl(Location, "Illegal reference to signal within a signal: '%s'", id.ToString(buf, sizeof(buf)));
				sigref.ResolvedSignal = NULL;
				sigref.ResolvedInstance = NULL;
				return false;
			}

			// Signal found, and because it was found on the way up, it is not associated with an instance
			sigref.ResolvedSignal = sig;
			sigref.ResolvedInstance = NULL;
			return true;
		}

		// Check for an instance with the corresponding name
		Instance *inst = module->GetInstance(firstId);
		if (inst)
		{
			// If an instance is found, then the dotted identifier must not be complete
			// It is illegal to refer to an instance in a connection
			if (id.Next == NULL)
			{
				char buf[256];
				yyerrorfl(Location, "Illegal reference to an instance.  Connections must refer to signals: '%s'", id.ToString(buf, sizeof(buf)));
				sigref.ResolvedSignal = NULL;
				sigref.ResolvedInstance = NULL;
				return false;
			}
			else
			{
				// Look downwards into the instance
				if (inst->Definition)
				{
					// Resolved the instance, continue looking for the signal

					// FIXME: this is implemented as though deep references to signals inside of instances were supported,
					// however the Connection class only supports a single level of instance hierarchy,
					// to support immediate connections to ports
					sigref.ResolvedInstance = inst;
					return LookDownForSignal(inst->Definition, *id.Next, sigref);
				}

				yyerrorfl(Location, "Instance '%s' in module '%s' does not have a definition", inst->Name(), module->Name());
				sigref.ResolvedSignal = NULL;
				sigref.ResolvedInstance = NULL;
				return false;
			}
		}

		// Keep searching upwards
		module = module->ParentModule();
	}

	// If anchor not found in any module scope, the signal does not exist
	if (id.Next)
	{
		char buf[256];
		yyerrorfl(Location, "Instance '%s' not found.  Cannot resolve signal '%s'", firstId, id.ToString(buf, sizeof(buf)));
	}
	else
	{
		yyerrorfl(Location, "Signal '%s' not found", firstId);
	}

	sigref.ResolvedSignal = NULL;
	sigref.ResolvedInstance = NULL;
	return false;
}



bool Connection::LookDownForSignal(const Module *module, const DottedIdentifier &id, SignalReference &sigref) const
{
	Signal *sig = module->GetSignal(id.Name);

	if (sig)
	{
		// If a signal is found, then the dotted identifier must be complete
		// Currently, there is no signal nesting supprted (i.e. structs or bundles)
		if (id.Next)
		{
			char buf[256];
			yyerrorfl(Location, "Illegal reference to signal within a signal: '%s'", id.ToString(buf, sizeof(buf)));
			sigref.ResolvedSignal = NULL;
			sigref.ResolvedInstance = NULL;
			return false;
		}

		// Signal found
		sigref.ResolvedSignal = sig;
		return true;
	}


	// Check for an instance with the corresponding name
	Instance *inst = module->GetInstance(id.Name);
	if (inst)
	{
		// If an instance is found, then the dotted identifier must not be complete
		// It is illegal to refer to an instance in a connection
		if (id.Next == NULL)
		{
			yyerrorfl(Location, "Illegal reference to an instance.  Connections must refer to signals");
			sigref.ResolvedSignal = NULL;
			sigref.ResolvedInstance = NULL;
			return false;
		}
		else
		{
			// Look downwards into the instance
			if (inst->Definition)
			{
				// FIXME: One day supporting references deep into instances could be supported,
				// by simply removing this statement and appending to a deeper list of instances.
				// However, more complex signal flattening logic would also have to be implemented.
				if (sigref.ResolvedInstance)
				{
					yyerrorfl(Location, "Cannot refer to a signal deeper than one level inside of a module");
					sigref.ResolvedSignal = NULL;
					sigref.ResolvedInstance = NULL;
					return false;
				}

				// Found the instance, continue on to obtain the signal
				sigref.ResolvedInstance = inst;
				return LookDownForSignal(inst->Definition, *id.Next, sigref);
			}

			yyerrorfl(Location, "Instance '%s' in module '%s' does not have a definition", inst->Name(), module->Name());
			sigref.ResolvedSignal = NULL;
			sigref.ResolvedInstance = NULL;
			return false;
		}
	}
	
	// If anchor not found in any module scope, the signal does not exist
	if (id.Next)
	{
		char buf[256];
		yyerrorfl(Location, "Instance '%s' not found.  Cannot resolve signal: '%s'", id.Name, id.ToString(buf, sizeof(buf)));
	}
	else
	{
		yyerrorfl(Location, "Signal '%s' not found", id.Name);
	}

	sigref.ResolvedSignal = NULL;
	sigref.ResolvedInstance = NULL;
	return false;
}


// Recursively modifies the connection, creating additional wires, ports, and connections
// until each connection meets one of the following criteria:
//   - From a signal to another signal in the same module
//   - From an output port to a wire in the same module as the instance
//   - From a wire to an input port of an instance in the same module as the wire
// Returns true if connection still remains, and false if the connection has been marked for deletion
bool Connection::Flatten()
{
	// In order to be considered flat, both the source and destination must be local,
	// which means either a local signal or a port of a local instance.
	// Even if both are local, however, they cannot both be connections to ports.

	// First the source and destination are made local by creating a local port for each
	// connection to a module at a higher level of hierarchy, creating an OuterConnection
	// which will be used to automatically wire up the OuterConnection from instances.

	// Then, if necessary, an automatic local wire is made between local ports.

	// If the connection is from an outer signal to an input port, or from an output port to an outer signal,
	// then this connection needs to be removed, after setting up an OuterConnection.
	bool deleted = false;

	if (Source.ResolvedInstance == NULL)
	{
		// Source is a declared signal, not wired from a port
		// Check for NULL module is for built-in signals, which are considered local
		if (Source.ResolvedSignal->module == NULL || Source.ResolvedSignal->module == module)
		{
			// Source is a locally declared signal
		}
		else
		{
			// Source is declared in a parent module

			if (Destination.ResolvedInstance == NULL && Destination.ResolvedSignal->module == module && Destination.ResolvedSignal->Direction == DIR_IN)
			{
				// Destination is to an input in the current module, which means the signal was declared with outer syntax to an input
				// Add a new outer connection to the current module, from the source to this port
				// Mark this connection for deletion

				OuterConnection *outer = new OuterConnection(module, DIR_IN, Source.ResolvedSignal, NULL, Destination.ResolvedSignal, NULL);
				module->AddOuterConnection(outer);
				deleted = true;
			}
			else
			{
				// Create an input port in the current module with the name "module$signal"
				// Add a new outer connection to the current module, from the original source to the new port
				// Modify this connection to come from the new port

				// Create port name
				strings->StartString();
				strings->AppendString(Source.ResolvedSignal->module->Name());
				strings->AppendChar('$');
				strings->AppendString(Source.ResolvedSignal->Name());
				const char *portName = strings->FinishString();

				// Get existing or create new input port
				Signal *port = module->GetSignal(portName);
				if (port)
				{
					// Port has already been added
				}
				else
				{
					port = new Signal(portName, BEHAVIOR_WIRE, Source.ResolvedSignal->DataType, DIR_IN);
					port->Automatic = true;
					module->AddSignal(port);
				}

				// Create a new outer connection input from the original source, to the new port
				OuterConnection *outer = new OuterConnection(module, DIR_IN, Source.ResolvedSignal, NULL, port, NULL);
				module->AddOuterConnection(outer);

				// Modify connection to come from the new port
				Source.ResolvedSignal = port;
				Source.ResolvedInstance = NULL;
			}
		}
	}
	else
	{
		// Source is a connection from a port
		// Check for NULL module is for built-in signals, which are considered local
		if (Source.ResolvedSignal->module == NULL || Source.ResolvedInstance->module == module)
		{
			// Source is a connection from a locally declared instance
		}
		else
		{
			// Source is a connection from an instance in a parent module

			if (Destination.ResolvedInstance == NULL && Destination.ResolvedSignal->module == module && Destination.ResolvedSignal->Direction == DIR_IN)
			{
				// Destination is to an input in the current module, which means the signal was declared with outer syntax to an input
				// Add a new outer connection to the current module, from the source to this port
				// Mark this connection for deletion

				OuterConnection *outer = new OuterConnection(module, DIR_IN, Source.ResolvedSignal, Source.ResolvedInstance, Destination.ResolvedSignal, NULL);
				module->AddOuterConnection(outer);
				deleted = true;
			}
			else
			{
				// Create an input port with the name "module$instance$signal"
				// Add a new outer connection, from the original source to the new port
				// Modify this connection to come from the new port

				// Create port name
				strings->StartString();
				strings->AppendString(Source.ResolvedInstance->module->Name());
				strings->AppendChar('$');
				strings->AppendString(Source.ResolvedInstance->Name());
				strings->AppendChar('$');
				strings->AppendString(Source.ResolvedSignal->Name());
				const char *portName = strings->FinishString();

				// Get existing or create new input port
				Signal *port = module->GetSignal(portName);
				if (port)
				{
					// Port has already been added
				}
				else
				{
					port = new Signal(portName, BEHAVIOR_WIRE, Source.ResolvedSignal->DataType, DIR_IN);
					port->Automatic = true;
					module->AddSignal(port);
				}

				// Create a new outer connection input from the original source, to the new port
				OuterConnection *outer = new OuterConnection(module, DIR_IN, Source.ResolvedSignal, Source.ResolvedInstance, port, NULL);
				module->AddOuterConnection(outer);

				// Modify connection to come from the new port
				Source.ResolvedSignal = port;
				Source.ResolvedInstance = NULL;
			}
		}
	}


	// Destination is a wire
	if (Destination.ResolvedInstance == NULL)
	{
		// Destination is a declared signal, not wired from a port
		// Check for NULL module is for built-in signals, which are considered local
		if (Destination.ResolvedSignal->module == NULL || Destination.ResolvedSignal->module == module)
		{
			// Destination is a locally declared signal
		}
		else
		{
			// Destination is declared in a parent module

			if (Source.ResolvedInstance == NULL && Source.ResolvedSignal->module == module && Source.ResolvedSignal->Direction == DIR_OUT)
			{
				// Source is from an output in the current module, which means the signal was declared with outer syntax from an output
				// Add a new outer connection from the current module from this port to the destination
				// Mark this connection for deletion

				OuterConnection *outer = new OuterConnection(module, DIR_OUT, Source.ResolvedSignal, NULL, Destination.ResolvedSignal, NULL);
				module->AddOuterConnection(outer);
				deleted = true;
			}
			else
			{
				// Create an output port with the name "module$signal"
				// Add a new outer connection, from the new port to the original destination
				// Modify this connection to go to the new port

				// Create port name
				strings->StartString();
				strings->AppendString(Destination.ResolvedSignal->module->Name());
				strings->AppendChar('$');
				strings->AppendString(Destination.ResolvedSignal->Name());
				const char *portName = strings->FinishString();

				// Get existing or create new output port
				Signal *port = module->GetSignal(portName);
				if (port)
				{
					// Port has already been added
				}
				else
				{
					port = new Signal(portName, BEHAVIOR_WIRE, Destination.ResolvedSignal->DataType, DIR_OUT);
					port->Automatic = true;
					module->AddSignal(port);
				}

				// Create a new outer connection output to the original destination, from the new port
				OuterConnection *outer = new OuterConnection(module, DIR_OUT, port, NULL, Destination.ResolvedSignal, NULL);
				module->AddOuterConnection(outer);

				// Modify connection to go to the new port
				Destination.ResolvedSignal = port;
				Destination.ResolvedInstance = NULL;
			}
		}
	}
	else
	{
		// Destination is a connection to a port
		if (Destination.ResolvedSignal->module == NULL || Destination.ResolvedInstance->module == module)
		{
			// Destination is a connection to a locally declared instance
		}
		else
		{
			// Destination is a connection to an instance in a parent module

			if (Source.ResolvedInstance == NULL && Source.ResolvedSignal->module == module && Source.ResolvedSignal->Direction == DIR_OUT)
			{
				// Source is from an output in the current module, which means the signal was declared with outer syntax from an output
				// Add a new outer connection from the current module from this port to the destination
				// Mark this connection for deletion

				OuterConnection *outer = new OuterConnection(module, DIR_OUT, Source.ResolvedSignal, NULL, Destination.ResolvedSignal, Destination.ResolvedInstance);
				module->AddOuterConnection(outer);
				deleted = true;
			}
			else
			{
				// Create an output port with the name "module$instance$signal"
				// Add a new outer connection, from the new port to the original destination
				// Modify this connection to go to the new port

				// Create port name
				strings->StartString();
				strings->AppendString(Destination.ResolvedInstance->module->Name());
				strings->AppendChar('$');
				strings->AppendString(Destination.ResolvedInstance->Name());
				strings->AppendChar('$');
				strings->AppendString(Destination.ResolvedSignal->Name());
				const char *portName = strings->FinishString();

				// Get existing or create new output port
				Signal *port = module->GetSignal(portName);
				if (port)
				{
					// Port has already been added
				}
				else
				{
					port = new Signal(portName, BEHAVIOR_WIRE, Destination.ResolvedSignal->DataType, DIR_OUT);
					port->Automatic = true;
					module->AddSignal(port);
				}

				// Create a new outer connection output to the original destination, from the new port
				OuterConnection *outer = new OuterConnection(module, DIR_OUT, port, NULL, Destination.ResolvedSignal, Destination.ResolvedInstance);
				module->AddOuterConnection(outer);

				// Modify connection to go to the new port
				Destination.ResolvedSignal = port;
				Destination.ResolvedInstance = NULL;
			}
		}
	}

	
	// The above section may mark this connection for deletion.  If so, delete self, and return
	if (deleted)
	{
		module->RemoveConnection(this);
		delete this;
		return false;
	}


	// At this point, the hierarchy has been flattened.  The connection no longer contains references to signals
	// in another module, only local signals or ports on local instances.

	// If the connection is between two local instance ports,
	// or if the connection is from a source instance port which is delayed or bit-sliced,
	// then create a local wire for the source
	if (Source.ResolvedInstance && (Destination.ResolvedInstance || (Source.DelayCount != 0) || Source.VBit))
	{
		// Create an automatic local wire with the name "instance$signal"
		// Add a new connection from the original source to the automatic wire
		// Modify this source to come from the automatic wire

		// Create wire name
		strings->StartString();
		strings->AppendString(Source.ResolvedInstance->Name());
		strings->AppendChar('$');
		strings->AppendString(Source.ResolvedSignal->Name());
		const char *wireName = strings->FinishString();

		// Get existing or create new local wire
		Signal *wire = module->GetSignal(wireName);
		if (wire)
		{
			// Wire has already been added
		}
		else
		{
			wire = new Signal(wireName, BEHAVIOR_WIRE, Source.ResolvedSignal->DataType, DIR_NONE);
			wire->Automatic = true;
			module->AddSignal(wire);
		}

		// Add a new connection from the original source to the automatic wire
		Connection *c = new Connection(module, Source.ResolvedSignal, Source.ResolvedInstance, wire, NULL, Location);
		module->AddConnection(c);

		// Modify this source to come from the the automatic wire
		// Maintain the original destination
		// Any DelayCount or VBit properties on this connection will remain,
		// which logically moves the delay or bit-slice downstream toward the destination
		Source.ResolvedSignal = wire;
		Source.ResolvedInstance = NULL;
	}


	// If the connection is from a delayed signal or is bit-sliced, then resolve this here.
	// The above section guarantees that such a signal will be local, and not an instance port.

	// Always delay before bit-slicing
	if (Source.DelayCount != 0)
	{
		Source.ResolvedSignal = Source.ResolvedSignal->Delay(Source.DelayCount);
		Source.DelayCount = 0;
	}

	if (Source.VBit)
	{
		Source.ResolvedSignal = Source.ResolvedSignal->VBit();
		Source.VBit = false;
	}

	return true;
}



// OuterConnection struct, used during ResolveConnections phase to create ports and connections in inner module hierarchies
OuterConnection::OuterConnection()
	: module(NULL), Direction(DIR_NONE), SourceSignal(NULL), SourceInstance(NULL), DestinationSignal(NULL), DestinationInstance(NULL)
{
}

OuterConnection::OuterConnection(Module *module, SignalDirection direction, Signal *sourceSignal, Instance *sourceInstance, Signal *destinationSignal, Instance *destinationInstance)
	: module(module), Direction(direction), SourceSignal(sourceSignal), SourceInstance(sourceInstance), DestinationSignal(destinationSignal), DestinationInstance(destinationInstance)
{
}


void OuterConnection::Print(FILE *f) const
{
	fprintf(f, "outer ");

	if (Direction == DIR_IN)
		fprintf(f, "input ");
	else if (Direction == DIR_OUT)
		fprintf(f, "output ");

	if (SourceSignal)
	{
		if (SourceInstance)
		{
			if (SourceInstance->module)
			{
				if (SourceInstance->module != module)
					fprintf(f, "%s:", SourceInstance->module->Name());
			}
			fprintf(f, "%s.", SourceInstance->Name());
		}
		else
		{
			if (SourceSignal->Behavior == BEHAVIOR_BUILTIN)
			{
				fprintf(f, "BUILTIN:");
			}
			else if (SourceSignal->module)
			{
				if (SourceSignal->module != module)
					fprintf(f, "%s:", SourceSignal->module->Name());
			}
		}

		fprintf(f, "%s", SourceSignal->Name());
	}

	fprintf(f, " -> ");

	if (DestinationSignal)
	{
		if (DestinationInstance)
		{
			if (DestinationInstance->module)
			{
				if (DestinationInstance->module != module)
					fprintf(f, "%s:", DestinationInstance->module->Name());
			}
			fprintf(f, "%s.", DestinationInstance->Name());
		}
		else
		{
			if (DestinationSignal->Behavior == BEHAVIOR_BUILTIN)
			{
				fprintf(f, "BUILTIN:");
			}
			else if (DestinationSignal->module)
			{
				if (DestinationSignal->module != module)
					fprintf(f, "%s:", DestinationSignal->module->Name());
			}
		}

		fprintf(f, "%s", DestinationSignal->Name());
	}
}



// Two outer connections are equal if their fields are equal
bool OuterConnection::operator==(const OuterConnection &outerConnection) const
{
	// The modules must both be the same
	if (module && outerConnection.module)
	{
		if (module != outerConnection.module)
			return false;
	}

	return (Direction == outerConnection.Direction) &&
		(SourceSignal == outerConnection.SourceSignal) &&
		(SourceInstance == outerConnection.SourceInstance) &&
		(DestinationSignal == outerConnection.DestinationSignal) &&
		(DestinationInstance == outerConnection.DestinationInstance);
}

bool OuterConnection::operator!=(const OuterConnection &outerConnection) const
{
	// Invert result of operator==
	return !(*this == outerConnection);
}
