
#ifndef VARIABLE_H
#define VARIABLE_H

#include "Symbol.h"
#include "Expression.h"

class Variable : public Symbol
{
public:
	Variable(const char *name);
	virtual ~Variable();

	virtual SymbolTypeId SymbolType() const { return SYMBOL_VARIABLE; }
	virtual bool Shadowable() const { return true; }
	virtual void Print(FILE *f) const;

	Expression Value;
};

#endif
