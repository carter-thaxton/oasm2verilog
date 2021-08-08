
#ifndef MODULE_H
#define MODULE_H

#include "Symbol.h"
#include "Signal.h"
#include "Parameter.h"
#include "Instance.h"
#include "Connection.h"
#include "StringMap.h"
#include "Common.h"

#include <vector>
using namespace::std;


class Module : public Symbol
{
public:
	Module(const char *name, Module *parent = NULL, bool isExtern = false);
	virtual ~Module();

	virtual SymbolTypeId SymbolType() const { return SYMBOL_MODULE; }
	virtual const char *ModuleTypeName() const { return "MODULE"; }

	// Used in report generation and debugging
	virtual void Print(FILE *f) const;

	// Keep track of the number of instances created with this definition
	int IncrementNumInstances();
	int NumInstances();

	// Get parent module definition, NULL if unnested
	Module *ParentModule() const;

	// Whether an extern definition
	bool IsExtern() const;

	// Whether a top-level FPOA
	virtual bool IsFPOA() const;

	// Signals by name and index
	int SignalCount() const;
	Signal *AddSignal(Signal *signal);
	Signal *GetSignal(const char *name) const;
	Signal *GetSignal(int i) const;

	// Parameters by name and index
	int ParameterCount() const;
	Parameter *AddParameter(Parameter *param);
	Parameter *GetParameter(const char *name) const;
	Parameter *GetParameter(int i) const;

	// Submodule instances by name and index
	int InstanceCount() const;
	Instance *AddInstance(Instance *instance);
	Instance *GetInstance(const char *name) const;
	Instance *GetInstance(int i) const;

	// Inner module definitions by name and index
	int InnerModuleCount() const;
	Module *AddInnerModule(Module *innerModule);
	Module *GetInnerModule(const char *name) const;
	Module *GetInnerModule(int i) const;

	// Structural connections by index
	int ConnectionCount() const;
	int ConnectionIndex(const Connection *connection) const;
	bool HasConnection(const Connection *connection) const;
	bool AddConnection(Connection *connection);
	bool RemoveConnection(Connection *connection);
	Connection *GetConnection(int i) const;

	// Outer connections, used during ResolveConnections phase
	int OuterConnectionCount() const;
	bool AddOuterConnection(OuterConnection *outerConnection);
	OuterConnection *GetOuterConnection(int i) const;

	// After parsing, perform analysis passes to apply default values,
	// assign resources, and check for errors
	bool AnalyzeAfterParse();       // Performed after parsing this module   (phase 1)
	bool ResolveInstances();		// Performed after parsing all modules   (phase 2)
	bool ResolveConnections();		// Performed after resolving instances on all modules   (phase 3)

	// Public field to store the source code location of the module definition
	SourceCodeLocation Location;


	// Defined in Module_GenerateVerilog

	// Primary Verilog generation method, overridden by various module types
	virtual void GenerateVerilog(FILE *f) const;

	// Generates an embedded comment containing an extern interface declaration
	// for the module or object
	virtual void GenerateVerilogEmbeddedExtern(FILE *f, bool suppressEmbeddedOasmDeclarations = false) const;

	// Used to generate a header at the top of each Verilog output file
	static  void GenerateVerilogHeader(FILE *f);


protected:
	// Calls made during AnalyzeAfterParse
	virtual bool ApplyDefaultValues();
	virtual bool AssignResources();

	// Look up module in local scope first, and work upwards to global scope
	Module *LookupModule(const char *moduleName) const;

	// Called from within ResolveConnections
	bool ResolveConnectionsPass2();
	bool CheckConnections() const;

	// Hook for extra connections during ResolveConnections, before flattening.
	// Some SiliconObjects perform extra connections, like the RF
	virtual bool ExtraResolveConnections();

	// Defined in Module_GenerateVerilog
	virtual void GenerateVerilogModuleName(FILE *f) const;
	virtual void GenerateVerilogWires(FILE *f) const;
	virtual void GenerateVerilogConnections(FILE *f) const;
	virtual void GenerateVerilogInstances(FILE *f) const;
	virtual void GenerateVerilogDelays(FILE *f) const;
	static  void GenerateVerilogDelay(FILE *f, const Signal *delayedSignal);


private:
	Module *parent;
	bool isExtern;
	int numInstances;

	StringMap signals;
	StringMap parameters;
	StringMap instances;
	StringMap innerModules;

	vector<Connection*> connections;
	vector<OuterConnection*> outerConnections;
};

#endif
