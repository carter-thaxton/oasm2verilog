
#ifndef FUNCTION_H
#define FUNCTION_H

#include "Symbol.h"
#include "Expression.h"

class SymbolTable;

// Base class for BuiltinFunction (called at compile time) and AluInstructionCall
class Function : public Symbol
{
public:
	Function(const char *name, const char *args = NULL);
	virtual ~Function();

	// Override Key function to mangle name as "name__args" or "name__0" if no args
	virtual const char *Key() const;

	int NumArgs() const;
	char ArgType(int arg) const;

	virtual SymbolTypeId SymbolType() const { return SYMBOL_FUNCTION; }

	virtual void Print(FILE *f) const;

	virtual Expression Evaluate() const;
	virtual Expression Evaluate(const Expression &op1) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const;

	bool CheckArgumentType(int arg, const Expression &op) const;

	static Expression Call(SymbolTable *symbols, const char *fcn_name, int nargs, const Expression *op1, const Expression *op2, const Expression *op3, const Expression *op4);
	static const char *TypeStringFromChar(char c);

protected:
	char *key;
	const char *args;
};

#endif
