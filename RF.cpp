
#include "RF.h"
#include "Common.h"

RF_RAM::RF_RAM(const char *name, Module *parent)
	: SiliconObject(name, parent)
{
}

RF_RAM::~RF_RAM()
{
}


// Hook for extra connections during ResolveConnections, before flattening.
// When wr_width is 1, RF_RAM hooks up the wr_data1 ports identically to the wr_data0 ports
bool RF_RAM::ExtraResolveConnections()
{
	int wr_width = 1;

	Parameter *wr_width_param = GetParameter("wr_width");
	if (wr_width_param)
	{
		wr_width = wr_width_param->Value.ToInt();
	}

	if (wr_width == 1)
	{
		// Perform extra logic to automatically connect up wr_data1 ports to the wr_data0 ports.
		// If they are explicitly connected, check that they are identical to the wr_data0 ports.
	}

	return true;
}



/*
 * Singleton instance of definition, created on-demand
 */

// Factory method to create RF_RAM
static SiliconObject *RF_RAM_Factory(const SiliconObjectDefinition *definition, const char *name, Module *parent)
{
	return new RF_RAM(name, parent);
}


const SiliconObjectDefinition *RF_RAM::Definition() const
{
	// Use built-in static method
	return BuiltinDefinition();
}

const SiliconObjectDefinition *RF_RAM::BuiltinDefinition()
{
	// Return singleton instance if already created
	if (definition)
		return definition;

	// Create a new builtin singleton symbol table instance
	SymbolTable *symbols = new SymbolTable(true);


	// RF_RAM Signals
	symbols->Add(new Signal("wr",								BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_addr",							BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_IN ));

	symbols->Add(new Signal("wr_data0_word",					BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_IN ));
	symbols->Add(new Signal("wr_data0_tag0",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data0_tag1",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data0_tag2",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data0_tag3",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));

	symbols->Add(new Signal("be_wr_data0_word_bits15to8",       BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("be_wr_data0_word_bits7to0",        BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("be_wr_data0_tags",                 BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));

	symbols->Add(new Signal("wr_data1_word",					BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_IN ));
	symbols->Add(new Signal("wr_data1_tag0",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data1_tag1",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data1_tag2",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("wr_data1_tag3",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));

	symbols->Add(new Signal("be_wr_data1_word_bits15to8",       BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("be_wr_data1_word_bits7to0",        BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("be_wr_data1_tags",                 BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));


	symbols->Add(new Signal("rd",								BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("rd_addr",							BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_IN ));

	symbols->Add(new Signal("rd_data0_word",					BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_OUT ));
	symbols->Add(new Signal("rd_data0_tag0",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data0_tag1",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data0_tag2",					BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data0_tag3",				    BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));

	symbols->Add(new Signal("rd_data1_word",					BEHAVIOR_BUILTIN, DATA_TYPE_WORD, DIR_OUT ));
	symbols->Add(new Signal("rd_data1_tag0",				    BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data1_tag1",				    BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data1_tag2",				    BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));
	symbols->Add(new Signal("rd_data1_tag3",				    BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_OUT ));


	symbols->Add(new Signal("flush",							BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));
	symbols->Add(new Signal("warm_reset",						BEHAVIOR_BUILTIN, DATA_TYPE_BIT,  DIR_IN ));


	// RF_RAM_Parameters
	const char *high_low[] =            {"active_high", "active_low"};
	const char *always_never_high[] =   {"active_always", "active_never", "active_high"};

	symbols->Add(new ParameterDefinition("wr_width",    1, 2, 1));
	symbols->Add(new ParameterDefinition("wr_mode",     high_low, countof(high_low), "active_high"));
	symbols->Add(new ParameterDefinition("rd_width",    1, 2, 1));
	symbols->Add(new ParameterDefinition("rd_mode",     high_low, countof(high_low), "active_high"));

	symbols->Add(new ParameterDefinition("be_wr_data0_word_bits15to8_mode",     always_never_high, countof(always_never_high), "active_always"));
	symbols->Add(new ParameterDefinition("be_wr_data0_word_bits7to0_mode",      always_never_high, countof(always_never_high), "active_always"));
	symbols->Add(new ParameterDefinition("be_wr_data0_tags_mode",               always_never_high, countof(always_never_high), "active_always"));
	symbols->Add(new ParameterDefinition("be_wr_data1_word_bits15to8_mode",     always_never_high, countof(always_never_high), "active_always"));
	symbols->Add(new ParameterDefinition("be_wr_data1_word_bits7to0_mode",      always_never_high, countof(always_never_high), "active_always"));
	symbols->Add(new ParameterDefinition("be_wr_data1_tags_mode",               always_never_high, countof(always_never_high), "active_always"));

	// array of 0-64 20-bit values
	symbols->Add(new ParameterDefinition("init_data",   -(1<<(20-1)), (1<<20)-1,  0, 64,  0xFFFFF));


	// Create a new singleton definition
	definition = new SiliconObjectDefinition("RF_RAM", symbols, RF_RAM_Factory);

	return definition;
}

SiliconObjectDefinition *RF_RAM::definition = NULL;

