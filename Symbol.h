
#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdio.h>

enum SymbolTypeId
{
	SYMBOL_UNKNOWN,
	SYMBOL_VARIABLE,
	SYMBOL_FUNCTION,
	SYMBOL_ALU_INST,
	SYMBOL_SIGNAL,
	SYMBOL_PARAMETER,
	SYMBOL_ENUM_VALUE,
	SYMBOL_MODULE,
	SYMBOL_INSTANCE,
};


class Symbol
{
protected:
	Symbol(const char *name);
public:
	virtual ~Symbol();

	// Get symbol name
	const char *Name() const;

	// By default, Key is Name, but some classes like Function override this to create mangled names
	virtual const char *Key() const;

	virtual SymbolTypeId SymbolType() const { return SYMBOL_UNKNOWN; }
	virtual bool Shadowable() const { return false; }
	virtual void Print(FILE *f) const;

protected:
	const char *name;
};

#endif
