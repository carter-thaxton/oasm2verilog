
#ifndef ALU_FUNCTION_H
#define ALU_FUNCTION_H

#include "Function.h"
#include "Expression.h"
#include "AluFunctionCall.h"

class SymbolTable;

class AluFunction : public Function
{
public:
	AluFunction(const char *name, const char *args = NULL);
	virtual ~AluFunction();

	virtual SymbolTypeId SymbolType() const { return SYMBOL_ALU_INST; }
	virtual void Print(FILE *f) const;

	virtual Expression Evaluate() const;
	virtual Expression Evaluate(const Expression &op1) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const;

private:
	static AluFunctionArg ToAluFunctionArg(const Expression &op, char argType);
};

#endif
