
#ifndef INSTANCE_H
#define INSTANCE_H

#include "Symbol.h"
#include "Common.h"

#include <vector>
using namespace std;

class Module;
class Connection;

class Instance : public Symbol
{
public:
	Instance(Module *module, const char *name, const char *definitionName = NULL);
	Instance(Module *module, const char *name, Module *definition);

	virtual ~Instance();

	// Overridden from Symbol
	virtual SymbolTypeId SymbolType() const { return SYMBOL_INSTANCE; }
	virtual bool Shadowable() const { return true; }
	virtual void Print(FILE *f) const;

	// Once all module definitions are parsed, each instance is associated with a definition
	// during ResolveInstances.  Until then, carrying around the definition name is sufficient.
	Module *module;
	const char *DefinitionName;
	Module *Definition;

	SourceCodeLocation Location;

	// Adds a resolved connection to one of the ports of this instance
	// Returns true if successful, or reports an error and returns false if the connection
	// is invalid, such as if more than one connection is made to an input port
	int ConnectionCount() const;
	bool AddConnection(Connection *connection);
	Connection *GetConnection(int i) const;

	// Checks whether all input and output ports of the instance are connected, and issues
	// a warning for each unconnected port.  Returns true if no unconnected ports
	bool CheckConnections() const;

private:
	vector<Connection*> connections;
};

#endif
