
#include "FPOA.h"
#include "Common.h"

FPOA::FPOA(const char *name, Module *parent)
	: SiliconObject(name, parent)
{
}

FPOA::~FPOA()
{
}

bool FPOA::IsFPOA() const
{
	return true;
}


// After parsing FPOA, perform checks to ensure that it truly defines the top of an FPOA design.
// This means that it may not be an inner module, and it must contain no input or output ports.
bool FPOA::AssignResources()
{
	if (ParentModule())
	{
		yyerrorfl(Location, "Top-level FPOA object '%s' may not be defined as an inner module", Name());
		return false;
	}

	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Direction != DIR_NONE && sig->Behavior != BEHAVIOR_BUILTIN)
		{
			yyerrorfl(sig->Location, "Top-level FPOA object '%s' may not declare any input or output ports", Name());
			return false;
		}
	}

	return true;
}


/*
 * Singleton instance of definition, created on-demand
 */

// Factory method to create FPOA
static SiliconObject *FPOA_Factory(const SiliconObjectDefinition *definition, const char *name, Module *parent)
{
	return new FPOA(name, parent);
}


const SiliconObjectDefinition *FPOA::Definition() const
{
	// Use built-in static method
	return BuiltinDefinition();
}

const SiliconObjectDefinition *FPOA::BuiltinDefinition()
{
	// Return singleton instance if already created
	if (definition)
		return definition;

	// Create a new builtin singleton symbol table instance
	SymbolTable *symbols = new SymbolTable(true);


	// FPOA_CONTROL Signals
	symbols->Add(new Signal("core_clock",						BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_NONE ));


	// FPOA_CONTROL Parameters
	const char *devices[] =				{"moa3600mx", "moa3600vx"};
	long core_clk_freqs[] =				{600, 800, 1000, 1200};

	symbols->Add(new ParameterDefinition("fpoa_base_dir",	"./"));
	symbols->Add(new ParameterDefinition("device",			devices, countof(devices), "moa3600mx"));
	symbols->Add(new ParameterDefinition("core_clk_freq",	core_clk_freqs, countof(core_clk_freqs), 1000));


	// Create a new singleton definition
	definition = new SiliconObjectDefinition("FPOA", symbols, FPOA_Factory);

	return definition;
}

SiliconObjectDefinition *FPOA::definition = NULL;

