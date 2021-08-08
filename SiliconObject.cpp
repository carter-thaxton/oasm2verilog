
#include "SiliconObject.h"


//
// SiliconObject
//
SiliconObject::SiliconObject(const char *name, Module *parent)
	: Module(name, parent)
{
}

SiliconObject::~SiliconObject()
{
}

// The module type name is derived from the definition name
const char *SiliconObject::ModuleTypeName() const
{
	const SiliconObjectDefinition *defn = Definition();
	if (defn)
		return defn->Name();
	else
		return "UNKNOWN";
}



//
// SimpleSiliconObject
//
SimpleSiliconObject::SimpleSiliconObject(const SiliconObjectDefinition *definition, const char *name, Module *parent)
	: SiliconObject(name, parent), definition(definition)
{
}

SimpleSiliconObject::~SimpleSiliconObject()
{
}

// Definition of SimpleSiliconObject is simply carried by pointer in each instance
const SiliconObjectDefinition *SimpleSiliconObject::Definition() const
{
	return definition;
}



//
// SiliconObjectDefinition
//
SiliconObjectDefinition::SiliconObjectDefinition(const char *name, SymbolTable *symbols, SiliconObjectFactory factory)
	: Symbol(name), symbols(symbols), factory(factory)
{
}

SiliconObjectDefinition::~SiliconObjectDefinition()
{
	// Delete built-in symbol table when definition is destroyed
	if (symbols)
	{
		delete symbols;  symbols = NULL;
	}
}

SymbolTable *SiliconObjectDefinition::Symbols() const
{
	return symbols;
}

SiliconObject *SiliconObjectDefinition::Create(const char *name, Module *parent) const
{
	// When a factory is registered, use it to create a subclass of SiliconObject
	// When no factory is registered, create a SimpleSiliconObject

	if (factory)
	{
		return factory(this, name, parent);
	}
	else
	{
		return new SimpleSiliconObject(this, name, parent);
	}
}
