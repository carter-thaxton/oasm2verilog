
#include "Module.h"
#include "parser.h"

Module::Module(const char *name, Module *parent, bool isExtern)
	: Symbol(name), parent(parent), isExtern(isExtern), numInstances(0)
{
	if (parent)
		parent->AddInnerModule(this);
}

Module::~Module()
{
	// Modules are the primary owner of Signals, Parameters, Instances, Connections, OuterConnections, and recursive InnerModules

	// Signals
	int nsignals = SignalCount();
	for (int i=0; i < nsignals; i++)
	{
		Signal *sig = GetSignal(i);
		delete sig;
	}

	// Parameters
	int nparams = ParameterCount();
	for (int i=0; i < nparams; i++)
	{
		Parameter *param = GetParameter(i);
		delete param;
	}

	// Instances
	int ninst = InstanceCount();
	for (int i=0; i < ninst; i++)
	{
		Instance *inst = GetInstance(i);
		delete inst;
	}

	// Connections
	int ncon = ConnectionCount();
	for (int i=0; i < ncon; i++)
	{
		Connection *con = GetConnection(i);
		delete con;
	}

	// OuterConnections
	int noutercon = OuterConnectionCount();
	for (int i=0; i < noutercon; i++)
	{
		OuterConnection *outercon = GetOuterConnection(i);
		delete outercon;
	}


	// Inner Modules, recursive
	int nmod = InnerModuleCount();
	for (int i=0; i < nmod; i++)
	{
		Module *mod = GetInnerModule(i);
		delete mod;
	}

	// Null out reference to parent module
	parent = NULL;

	// StringMaps will delete after destructor
}


// Keep track of the number of instances created with this definition
int Module::IncrementNumInstances()
{
	numInstances++;
	return numInstances;
}

int Module::NumInstances()
{
	return numInstances;
}


// Return parent at outer scope.  NULL if an unnested module
Module *Module::ParentModule() const
{
	return parent;
}

// Return whether an extern definition
bool Module::IsExtern() const
{
	return isExtern;
}

// Return whether a top-level FPOA.  FPOA object overrides this to return true.
bool Module::IsFPOA() const
{
	return false;
}


// Signal List
int Module::SignalCount() const
{
	return signals.Count();
}

Signal *Module::AddSignal(Signal *signal)
{
	const char *signalName = signal->Name();

	// Parameters, signals, and instances all share the same namespace within the module
	if (GetParameter(signalName))
	{
		yyerrorf("Cannot add signal '%s' because it conflicts with a parameter name", signalName);
		return NULL;
	}

	if (GetInstance(signalName))
	{
		yyerrorf("Cannot add signal '%s' because it conflicts with a submodule instance name", signalName);
		return NULL;
	}

	Signal *existing = GetSignal(signalName);
	if (existing)
	{
		yyerrorf("Signal '%s' is already defined on line %d", signalName, existing->Location.Line);
		return NULL;
	}

	// Signal is already associated with a module.  This should never happen.
	if (signal->module)
		return NULL;

	// Add the signal to the list
	Signal *result = (Signal*) signals.Add(signalName, signal);

	// Associate the signal with this module
	if (result)
		result->module = this;

	return result;
}

Signal *Module::GetSignal(const char *name) const
{
	return (Signal*) signals.Get(name);
}

Signal *Module::GetSignal(int i) const
{
	return (Signal*) signals.Get(i);
}


// Parameter List
int Module::ParameterCount() const
{
	return parameters.Count();
}

Parameter *Module::AddParameter(Parameter *param)
{
	const char *paramName = param->Name();

	// Parameters, signals, and instances all share the same namespace within the module
	if (GetSignal(paramName))
	{
		yyerrorf("Cannot add parameter '%s' because it conflicts with a signal name", paramName);
		return NULL;
	}

	if (GetInstance(paramName))
	{
		yyerrorf("Cannot add parameter '%s' because it conflicts with a submodule instance name", paramName);
		return NULL;
	}

	Parameter *existing = GetParameter(paramName);
	if (existing)
	{
		yyerrorf("Parameter '%s' is already defined on line %d", paramName, existing->Location.Line);
		return NULL;
	}

	return (Parameter*) parameters.Add(param->Name(), param);
}

Parameter *Module::GetParameter(const char *name) const
{
	return (Parameter*) parameters.Get(name);
}

Parameter *Module::GetParameter(int i) const
{
	return (Parameter*) parameters.Get(i);
}


// Submodule Instance List
int Module::InstanceCount() const
{
	return instances.Count();
}

Instance *Module::AddInstance(Instance *instance)
{
	const char *instanceName = instance->Name();

	// Parameters, signals, and instances all share the same namespace within the module
	// However, inner module definitions and instances share the same name by design
	if (GetSignal(instanceName))
	{
		yyerrorf("Cannot add submodule instance '%s' because it conflicts with a signal name", instanceName);
		return NULL;
	}

	if (GetParameter(instanceName))
	{
		yyerrorf("Cannot add submodule instance '%s' because it conflicts with a parameter name", instanceName);
		return NULL;
	}

	Instance *existing = GetInstance(instanceName);
	if (existing)
	{
		yyerrorf("Instance '%s' is already defined on line %d", instanceName, existing->Location.Line);
		return NULL;
	}

	return (Instance*) instances.Add(instanceName, instance);
}

Instance *Module::GetInstance(const char *name) const
{
	return (Instance*) instances.Get(name);
}

Instance *Module::GetInstance(int i) const
{
	return (Instance*) instances.Get(i);
}


// InnerModule Definition List
int Module::InnerModuleCount() const
{
	return innerModules.Count();
}

Module *Module::AddInnerModule(Module *innerModule)
{
	const char *moduleName = innerModule->Name();

	// Parameters, signals, and instances all share the same namespace within the module
	// Because inner module definitions and instances share the same name by design,
	// the name should be checked against the parameters, signals, and existing instances
	if (GetSignal(moduleName))
	{
		yyerrorf("Cannot add inner module definition '%s' because it conflicts with a signal name", moduleName);
		return NULL;
	}

	if (GetParameter(moduleName))
	{
		yyerrorf("Cannot add inner module definition '%s' because it conflicts with a parameter name", moduleName);
		return NULL;
	}

	if (GetInstance(moduleName))
	{
		yyerrorf("Cannot add inner module definition '%s' because it conflicts with a submodule instance name", moduleName);
		return NULL;
	}

	Module *existing = GetInnerModule(moduleName);
	if (existing)
	{
		yyerrorf("Inner module '%s' is already defined on line %d", moduleName, existing->Location.Line);
		return NULL;
	}


	// Add the definition to the current module, and return its definition
	Module *definition = (Module*) innerModules.Add(innerModule->Name(), innerModule);
	return definition;
}

Module *Module::GetInnerModule(const char *name) const
{
	return (Module*) innerModules.Get(name);
}

Module *Module::GetInnerModule(int i) const
{
	return (Module*) innerModules.Get(i);
}


// Structural connections by index
int Module::ConnectionCount() const
{
	return connections.size();
}

int Module::ConnectionIndex(const Connection *connection) const
{
	int n = connections.size();
	for (int i=0; i < n; i++)
	{
		if (*(connections[i]) == *connection)
			return i;
	}
	return -1;
}

bool Module::HasConnection(const Connection *connection) const
{
	return (ConnectionIndex(connection) >= 0);
}

bool Module::AddConnection(Connection *connection)
{
	if (!HasConnection(connection))
	{
		connections.push_back(connection);
		return true;
	}
	return false;
}

bool Module::RemoveConnection(Connection *connection)
{
	int i = ConnectionIndex(connection);
	if (i < 0)
		return false;

	connections.erase(connections.begin() + i);
	return true;
}

Connection *Module::GetConnection(int i) const
{
	int n = connections.size();
	if (i >= 0 && i < n)
		return connections[i];

	return NULL;
}


// Outer connections by index
int Module::OuterConnectionCount() const
{
	return outerConnections.size();
}

bool Module::AddOuterConnection(OuterConnection *outerConnection)
{
	int n = outerConnections.size();

	// Check for existing outer connections
	for (int i=0; i < n; i++)
	{
		const OuterConnection *oc = outerConnections[i];
		if (*oc == *outerConnection)
			return false;
	}

	outerConnections.push_back(outerConnection);
	return true;
}

OuterConnection *Module::GetOuterConnection(int i) const
{
	int n = outerConnections.size();
	if (i >= 0 && i < n)
		return outerConnections[i];

	return NULL;
}



// Perform first analysis pass on module, after parsing the module
// - Apply default values to uninitialized registers
// - Assign registers to resources, such as wordN_reg, tfN_reg, kN, branchN
// - Check for resource allocation errors
// - Lookup and set branch indexes based on string labels
// - Returns true if successful
bool Module::AnalyzeAfterParse()
{
	// Apply default values to uninitialized registers
	if (!ApplyDefaultValues())
		return false;

	// Assign registers to resources, such as wordN_reg, tfN_reg, kN, branchN
	if (!AssignResources())
		return false;

	return true;
}

// Regardless of the module type, uninitialized registers or constants should get a zero value
bool Module::ApplyDefaultValues()
{
	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		Signal *sig = GetSignal(i);

		// reg and const signals that are uninitialized should get a zero value
		if (sig->Behavior == BEHAVIOR_REG || sig->Behavior == BEHAVIOR_CONST)
		{
			if (sig->InitialValue < 0)
				sig->InitialValue = 0;
		}
	}
	return true;
}

// Stub for base class that simply returns true
bool Module::AssignResources()
{
	return true;
}


// Look up module in local scope first, and work upwards to global module scope
Module *Module::LookupModule(const char *moduleName) const
{
	const Module *scope = this;
	while (scope)
	{
		Module *result = scope->GetInnerModule(moduleName);
		if (result)
			return result;

		scope = scope->parent;
	}

	// If not found in any module scope, look up in global module namespace
	return (Module *) modules.Get(moduleName);
}


// Perform second pass on module, after parsing all modules
bool Module::ResolveInstances()
{
	bool ok = true;

	// Associate module instances with their module definitions
	int ninst = InstanceCount();
	for (int i=0; i < ninst; i++)
	{
		Instance *inst = GetInstance(i);
		if (!inst->Definition)
		{
			// Look up module names in local scope first, and work upwards to global module scope
			Module *definition = LookupModule(inst->DefinitionName);
			if (!definition)
			{
				yyerrorfl(inst->Location, "Module '%s' is not defined", inst->DefinitionName);
				ok = false;
				break;
			}

			// Check for recursive definitions
			Module *checkRecursive = this;
			while (checkRecursive)
			{
				if (definition == checkRecursive)
				{
					yyerrorfl(inst->Location, "Module '%s' cannot be instantiated inside of its own definition", definition->Name());
					ok = false;
					definition = NULL;
					break;
				}
				checkRecursive = checkRecursive->parent;
			}

			// Check for illegal instantiation of top-level FPOA
			if (definition->IsFPOA())
			{
				yyerrorfl(inst->Location, "Top-level FPOA '%s' cannot be instantiated in any other module", definition->Name());
				ok = false;
				definition = NULL;
				break;
			}
			

			inst->Definition = definition;
			definition->IncrementNumInstances();
		}
	}

	// ResolveInstances on inner module definitions
	int nmodules = innerModules.Count();
	for (int i=0; i < nmodules; i++)
	{
		Module *innerModule = (Module *) innerModules.Get(i);
		if (innerModule)
		{
			if (!innerModule->ResolveInstances())
				ok = false;
		}
	}

	// Count instances created of inner module definitions, and warn if never used
	for (int i=0; i < nmodules; i++)
	{
		Module *innerModule = (Module *) innerModules.Get(i);
		if (innerModule)
		{
			int count = innerModule->NumInstances();
			if (count == 0)
			{
				yywarnfl(innerModule->Location, "Inner module '%s' is defined but never instantiated", innerModule->Name());
			}
		}
	}

	return ok;
}


// Perform third pass on module, after resolving all instances
bool Module::ResolveConnections()
{
	bool ok = true;

	// Wire up signals and ports by name, and check that each referenced signal exists
	// Walk through the list backwards because some connections may make calls to RemoveConnection()
	// which will move any connections in the vector at higher indices to a lower index
	int nconnections = ConnectionCount();
	for (int i=nconnections-1; i >= 0; i--)
	{
		Connection *c = GetConnection(i);
		if (!c->Resolve())
			ok = false;
	}


	// Recursively call ResolveConnections on inner module definitions
	int nmodules = InnerModuleCount();
	for (int i=0; i < nmodules; i++)
	{
		Module *innerModule = GetInnerModule(i);
		if (innerModule)
		{
			if (!innerModule->ResolveConnections())
				ok = false;
		}
	}


	// Call to ExtraResolveConnections hook.  Some SiliconObjects (like RF) perform some
	// intelligent connection logic, before flattening and hierarchical outer connections
	if (!ExtraResolveConnections())
		ok = false;


	// After resolving all connections, modules may still contain outer connections,
	// which need to be resolved by creating connections at levels outside each instance.
	// These must be resolved only after each definition in an inner module hierarchy has been seen.
	// Therefore ResolveConnectionsPass2 is recursive, and works from bottom up.  It is only called
	// from a top-level module.
	if (parent == NULL)
	{
		if (!ResolveConnectionsPass2())
			ok = false;
	}

	return ok;
}


// At each level of hierarchy, outer connections in any inner module definitions
// may need to be resolved to outer level signals or ports.  These need to be stitched up for each instance,
// from bottom up through the module definition hierarchy.
// ResolveConnectionsPass2 resolves any outer connections of local instances, and may create new outer connections
// to be resolved at yet higher levels.
// Finally, it connects signals to instances and performs a final check
bool Module::ResolveConnectionsPass2()
{
	bool ok = true;

	// ResolveConnectionsPass2 is recursive, and works from bottom up
	int nmodules = InnerModuleCount();
	for (int i=0; i < nmodules; i++)
	{
		Module *innerModule = GetInnerModule(i);
		if (innerModule)
		{
			if (!innerModule->ResolveConnectionsPass2())
				ok = false;
		}
	}


	// Resolve outer connections on each local instance
	int ninst = InstanceCount();
	for (int i=0; i < ninst; i++)
	{
		Instance *inst = GetInstance(i);

		// Get outer connections from definition of instance
		Module *defn = inst->Definition;
		if (defn)
		{
			int noutercon = defn->OuterConnectionCount();
			for (int j=0; j < noutercon; j++)
			{
				OuterConnection *oc = defn->GetOuterConnection(j);

				// Create new local connection in this module, where the instance is instantiated
				Connection *c = NULL;

				if (oc->Direction == DIR_IN)
				{
					// If the outer connection is an input, it should go to this instance
					c = new Connection(this,
						oc->SourceSignal, oc->SourceInstance,
						oc->DestinationSignal, inst,
						inst->Location);
				}
				else if (oc->Direction == DIR_OUT)
				{
					// If the outer connection is an output, it should come from this instance
					c = new Connection(this,
						oc->SourceSignal, inst,
						oc->DestinationSignal, oc->DestinationInstance,
						inst->Location);
				}
				else
				{
					// illegal outer connection
					yyfatalfl(inst->Location, "Illegal outer connection");
					break;
				}

				// Handle local rule that instance ports may not connect locally,
				// and add yet higher-level outer connections.
				c->Flatten();

				// Add the new connection locally, and connect it to or from the instance
				bool added = AddConnection(c);

				// If the connection is not used, because it was already added, delete it
				if (!added)
					delete c;
			}
		}
	}


	// Now that connections have been resolved, flattened, and intermediate wires or ports created,
	// which match the rules of Verilog, we can connect local signals to local instances.
	int nconnections = ConnectionCount();
	for (int i=0; i < nconnections; i++)
	{
		Connection *c = GetConnection(i);
		if (c->Source.ResolvedInstance)
		{
			Instance *inst = c->Source.ResolvedInstance;
			inst->AddConnection(c);
		}
		if (c->Destination.ResolvedInstance)
		{
			Instance *inst = c->Destination.ResolvedInstance;
			inst->AddConnection(c);
		}
	}


	// After resolving connections on the entire module, including inner modules and outer conections,
	// check the local connections to ensure no missing ports for any instances, and no wires that go nowhere.
	// Returns false if an error occurred.  Note: missing connections are simply warnings (even if -w is used),
	// and will still return true, which will allow compilation to continue.
	if (!CheckConnections())
		ok = false;

	return ok;
}


// This hook exists so that some SiliconObjects can perform extra intelligent connections
// before flattening and other connection resolution.
// The default behavior is to simply return true.  Note that returning false will cause ResolveConnections to return false.
bool Module::ExtraResolveConnections()
{
	return true;
}


// Check for data type mismatches in connections.
// Check for missing connections on each instance port.
// Also check for local wires that have no source or destination (TBD)
bool Module::CheckConnections() const
{
	// Extern modules have no connections, so skip this check
	if (IsExtern())
		return true;

	bool ok = true;

	// For each signal, check that there are not multiple drivers
	// Warn if there are no drivers for a wire
	int ncon = ConnectionCount();
	int nsignals = SignalCount();
	for (int i=0; i < nsignals; i++)
	{
		const Signal *sig = GetSignal(i);

		// Count the number of local drivers for each signal
		int numDrivers = 0;
		for (int j=0; j < ncon; j++)
		{
			const Connection *con = GetConnection(j);
			if (con->Destination.ResolvedSignal == sig)
				numDrivers++;

			// Give an error for each additional driver found
			if (numDrivers > 1)
			{
				yyerrorfl(con->Location, "Multiple drivers found for signal '%s'", sig->Name());
				ok = false;
			}
		}

		// Warn if a local wire has no drivers
		if (numDrivers == 0 && sig->Behavior == BEHAVIOR_WIRE)
		{
			if (sig->Direction == DIR_OUT)
				yywarnfl(sig->Location, "No connections made to output '%s'", sig->Name());
			else if (sig->Direction == DIR_NONE)
				yywarnfl(sig->Location, "No connections made to wire '%s'", sig->Name());
		}
	}


	// Check for missing connections on each instance
	// Note: these are simply warnings, and will not update 'ok'
	int ninst = InstanceCount();
	for (int i=0; i < ninst; i++)
	{
		const Instance *inst = GetInstance(i);
		inst->CheckConnections();
	}

	return ok;
}


//
// Print for report and debugging
//
void Module::Print(FILE *f) const
{
	const char *type_str = ModuleTypeName();
	const char *extern_str = isExtern ? "EXTERN " : "";

	if (parent)
		fprintf(f, "%s : %s%s (%s)", Name(), extern_str, type_str, parent->Name());
	else
		fprintf(f, "%s : %s%s", Name(), extern_str, type_str);

	fprintf(f, "  [%d instances]\n", numInstances);

	int nsignals = signals.Count();
	for (int i=0; i < nsignals; i++)
	{
		Signal *s = (Signal *) signals.Get(i);
		if (s)
		{
			fprintf(f, "\t");
			s->Print(f);
			fprintf(f, "\n");
		}
	}

	int nparams = parameters.Count();
	for (int i=0; i < nparams; i++)
	{
		Parameter *p = (Parameter *) parameters.Get(i);
		if (p)
		{
			fprintf(f, "\t");
			p->Print(f);
			fprintf(f, "\n");
		}
	}

	int ninst = instances.Count();
	for (int i=0; i < ninst; i++)
	{
		Instance *inst = (Instance *) instances.Get(i);
		if (inst)
		{
			fprintf(f, "\t");
			inst->Print(f);
			fprintf(f, "\n");
		}
	}

	int nconnections = connections.size();
	for (int i=0; i < nconnections; i++)
	{
		Connection *c = connections[i];
		fprintf(f, "\t");
		c->Print(f);
		fprintf(f, "\n");
	}

	int nouterconnections = outerConnections.size();
	for (int i=0; i < nouterconnections; i++)
	{
		OuterConnection *c = outerConnections[i];
		fprintf(f, "\t");
		c->Print(f);
		fprintf(f, "\n");
	}

	int nmodules = innerModules.Count();
	if (nmodules > 0)
		fprintf(f, "\n");
	for (int i=0; i < nmodules; i++)
	{
		Module *innerModule = (Module *) innerModules.Get(i);
		if (innerModule)
		{
			innerModule->Print(f);
			fprintf(f, "\n");
		}
	}

}

