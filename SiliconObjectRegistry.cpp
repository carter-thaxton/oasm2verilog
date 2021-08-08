
#include "SiliconObjectRegistry.h"
#include "SymbolTable.h"
#include "Parameter.h"
#include "EnumValue.h"
#include "parser.h"

#include "SiliconObject.h"
#include "FPOA.h"
#include "Alu.h"
#include "TF.h"
#include "RF.h"


/*
 *  Silicon object registry
 */


// Global registry of silicon objects
StringMap *siliconObjects;


// Lookup in silicon object registry
SiliconObjectDefinition *LookupObjectDefinition(const char *object)
{
	return (SiliconObjectDefinition *) siliconObjects->Get(object);
}


/*
 * Enumerated values for parameters from all objects
 */

static void AddEnumValuesForObject(const SiliconObjectDefinition *definition)
{
	SymbolTable *table = definition->Symbols();
	if (table == NULL) return;

	int n = table->Count();
	for (int i=0; i < n; i++)
	{
		Symbol *s = table->Get(i);
		if (s->SymbolType() == SYMBOL_PARAMETER)
		{
			ParameterDefinition *param = (ParameterDefinition *) s;
			if (param->DataType == PARAM_ENUM || param->DataType == PARAM_ENUM_OR_INT)
			{
				for (int j=0; j < param->NumEnumValues; j++)
				{
					const char *enumValue = param->EnumValues[j];
					globalSymbols->Add(new EnumValue(enumValue));
				}
			}
		}
	}
}


// Initialize enumerations for all silicon object types
static void InitializeSiliconObjectEnumValues()
{
	int n = siliconObjects->Count();
	for (int i=0; i < n; i++)
	{
		SiliconObjectDefinition *definition = (SiliconObjectDefinition *) siliconObjects->Get(i);
		AddEnumValuesForObject(definition);
	}
}



/*
 *  Cleanup global symbols
 */
void CleanupSiliconObjects()
{
	// Delete SiliconObject definitions
	int n = siliconObjects->Count();
	for (int i=0; i < n; i++)
	{
		SiliconObjectDefinition *definition = (SiliconObjectDefinition *) siliconObjects->Get(i);
		delete definition;
	}

	// Delete registry
	delete siliconObjects;
	siliconObjects = NULL;
}



/*
 * Create silicon object registry
 */

// Initialize the silicon object registry, and add all silicon objects
void InitializeSiliconObjects()
{
	siliconObjects = new StringMap();

	// Objects with special syntax
	siliconObjects->Add("FPOA",		(void*) FPOA::BuiltinDefinition());
	siliconObjects->Add("ALU",		(void*) Alu::BuiltinDefinition());
	siliconObjects->Add("TF",		(void*) FloatingTF::BuiltinDefinition());

	// Objects with special factories, but no special syntax
	siliconObjects->Add("RF_RAM",	(void*) RF_RAM::BuiltinDefinition());

	// Objects with no special support, just definitions


	// Go through each object in the registry, and add each enumerated value
	// ever accepted by a parameter as an EnumValue in the global symbol table
	InitializeSiliconObjectEnumValues();
}


