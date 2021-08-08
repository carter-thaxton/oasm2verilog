
#include "Instance.h"
#include "Module.h"
#include "Connection.h"


Instance::Instance(Module *module, const char *name, const char *definitionName)
	: Symbol(name), module(module), DefinitionName(definitionName), Definition(NULL)
{
	// Instantiate by definition name, to be associated later
}

Instance::Instance(Module *module, const char *name, Module *definition)
	: Symbol(name), module(module), DefinitionName(NULL), Definition(definition)
{
	// Instantiate a known definition
	if (Definition)
		DefinitionName = Definition->Name();
}

Instance::~Instance()
{
	module = NULL;
	DefinitionName = NULL;
	Definition = NULL;
}

void Instance::Print(FILE *f) const
{
	const char *definition_str = (Definition == NULL) ? "(Unknown)" : Definition->Name();
	fprintf(f, "%s : %s", Name(), definition_str);
}


int Instance::ConnectionCount() const
{
	return connections.size();
}

Connection *Instance::GetConnection(int i) const
{
	if (i >= 0 && i < (int) connections.size())
	{
		return connections[i];
	}

	return NULL;
}

// Adds a resolved connection to one of the ports of this instance
// Returns true if successful, or reports an error and returns false if the connection
// is invalid, such as if more than one connection is made to an input port
bool Instance::AddConnection(Connection *connection)
{
	bool output = (connection->Source.ResolvedInstance == this);
	bool input  = (connection->Destination.ResolvedInstance == this);

	// The connection must resolve to this instance
	if (!input && !output)
	{
		yyfatalfl(connection->Location, "Illegal connection to instance '%s'", Name());
		return false;
	}

	if (output)
	{
		// The connection signal must resolve to the same module as this instance
		if (connection->Source.ResolvedSignal->module != Definition)
		{
			yyfatalfl(connection->Location, "Illegal connection to instance '%s'", Name());
			return false;
		}

		// Check whether the connection is indeed from a signal declared as an output port
		if (connection->Source.ResolvedSignal->Direction != DIR_OUT)
		{
			yyerrorfl(connection->Location, "Illegal connection from '%s'.  It must be declared an output port", connection->Source.ResolvedSignal->Name());
			return false;
		}
	}

	if (input)
	{
		// The connection signal must resolve to the same module as this instance
		if (connection->Destination.ResolvedSignal->module != Definition)
		{
			yyfatalfl(connection->Location, "Illegal connection to instance '%s' of type '%s'", Name());
			return false;
		}

		// Check whether the connection is indeed to a signal declared as an input port
		if (connection->Destination.ResolvedSignal->Direction != DIR_IN)
		{
			yyerrorfl(connection->Location, "Illegal connection to '%s'.  It must be declared an input port", connection->Destination.ResolvedSignal->Name());
			return false;
		}

		// Check that no other connections have been made to this input port.  Fanin is illegal.
		int n = connections.size();
		for (int i=0; i < n; i++)
		{
			Connection *c = connections[i];
			if (c->Destination.ResolvedSignal == connection->Destination.ResolvedSignal)
			{
				// Another connection is already made to the same input port
				yyerrorfl(connection->Location, "Only one connection to input '%s' is allowed.  Another connection was made to this on line %d",
					connection->Destination.ResolvedSignal->Name(), c->Location.Line);

				return false;
			}
		}
	}

	// Connection is legal
	connections.push_back(connection);
	return true;
}


// Checks whether all input and output ports of the instance are connected, and issues
// a warning for each unconnected port.  Returns true if no unconnected ports
bool Instance::CheckConnections() const
{
	bool ok = true;

	int nsig = Definition->SignalCount();
	for (int i=0; i < nsig; i++)
	{
		const Signal *sig = Definition->GetSignal(i);

		// If signal is an input port, check that this instance has a connection with that destination
		if (sig->Direction == DIR_IN)
		{
			bool found = false;
			int ncon = connections.size();
			for (int j=0; j < ncon; j++)
			{
				Connection *c = connections[j];
				if (c->Destination.ResolvedSignal == sig)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				yywarnfl(Location, "Input port '%s' is unconnected on '%s'", sig->Name(), Name());
				ok = false;
			}
		}

		// If signal is an output port, check that this instance has a connection with that source
		if (sig->Direction == DIR_OUT)
		{
			bool found = false;
			int ncon = connections.size();
			for (int j=0; j < ncon; j++)
			{
				Connection *c = connections[j];
				if (c->Source.ResolvedSignal == sig)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				yywarnfl(Location, "Output port '%s' is unconnected on '%s'", sig->Name(), Name());
				ok = false;
			}
		}
	}

	return ok;
}
