
#ifndef ENUM_VALUE_H
#define ENUM_VALUE_H

#include "Symbol.h"
#include "Expression.h"

// Extremely simple symbol that represents an enumerated value
// It has no underlying value other than its own name

class EnumValue : public Symbol
{
public:
	EnumValue(const char *name);
	virtual ~EnumValue();

	virtual SymbolTypeId SymbolType() const { return SYMBOL_ENUM_VALUE; }
	virtual bool Shadowable() const { return true; }
	virtual void Print(FILE *f) const;

	Expression ToExpression() const;
};

#endif
