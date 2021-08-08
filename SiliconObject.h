
#ifndef SILICON_OBJECT_H
#define SILICON_OBJECT_H

#include "Module.h"
#include "SymbolTable.h"

class SiliconObject;
class SimpleSiliconObject;
class SiliconObjectDefinition;


// Factory method prototype to create silicon objects
typedef SiliconObject *(*SiliconObjectFactory)   (const SiliconObjectDefinition *definition, const char *name, Module *parent);


// Base class for all silicon objects (ALU, MAC, RF_RAM, and so on)
// There are several rules common to all SiliconObjects that go here.
//
// SiliconObjects come in two types:  simple and special
//
// - Simple SiliconObjects are those that merely use a symbol table to describe their
//   ports and parameters, with no specific grammar or syntax in OASM.
//
// - Special SiliconObjects are those with special syntax or rules in OASM.
//   Describing their ports and parameters is not sufficient.
//
class SiliconObject : public Module
{
public:
	SiliconObject(const char *name, Module *parent = NULL);
	virtual ~SiliconObject();

	// Module type names are obtained via their definition name
	virtual const char *ModuleTypeName() const;

	// All SiliconObjects supply a definition.
	virtual const SiliconObjectDefinition *Definition() const = 0;

	// Defined in SiliconObject_GenerateVerilog
	virtual void GenerateVerilog(FILE *f) const;
};


// Simple silicon objects, those not marked as "special", are not implemented as concrete subclasses of SiliconObject,
// but are simply SiliconObject instances with a reference to a SiliconObjectDefinition
class SimpleSiliconObject : public SiliconObject
{
public:
	SimpleSiliconObject(const SiliconObjectDefinition *definition, const char *name, Module *parent = NULL);
	virtual ~SimpleSiliconObject();

	virtual const SiliconObjectDefinition *Definition() const;

private:
	const SiliconObjectDefinition *definition;
};


// All built-in silicon objects have a definition
// Several silicon objects have very special support in the OASM grammar and generator
// These special objects include the ALU and TF  (maybe MUX and MAC in the future)
// Other objects, notably the periphery objects, are configured simply with ports and parameters,
// and have little extra support within the OASM grammar or language
class SiliconObjectDefinition : public Symbol
{
public:
	SiliconObjectDefinition(const char *name, SymbolTable *symbols, SiliconObjectFactory factory = NULL);
	virtual ~SiliconObjectDefinition();

	// Returns a built-in symbol table containing port and parameter definitions
	SymbolTable *Symbols() const;

	// Creates a new silicon object, either using the provided factory method,
	// or creating a simple silicon object
	SiliconObject *Create(const char *name, Module *parent) const;

private:
	SymbolTable *symbols;
	SiliconObjectFactory factory;
};


#endif
