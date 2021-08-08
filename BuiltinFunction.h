
#ifndef BUILTIN_FUNCTION_H
#define BUILTIN_FUNCTION_H

#include "Function.h"

class BuiltinIntegerFunction : public Function
{
public:
	BuiltinIntegerFunction(const char *name, const char *args = NULL, void *fcn = NULL);

	virtual Expression Evaluate() const;
	virtual Expression Evaluate(const Expression &op1) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const;
	virtual Expression Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const;

private:
	void *fcn;
};


// Returns the length of the expression
class BuiltinLengthFunction : public Function
{
public:
	BuiltinLengthFunction();
	virtual Expression Evaluate(const Expression &op1) const;
};


// Returns a shallow copy of the expression
class BuiltinCopyFunction : public Function
{
public:
	BuiltinCopyFunction();
	virtual Expression Evaluate(const Expression &op1) const;
};



// Global function called at parser initialization
// to add built-in functions to global symbol table
void InitializeBuiltinFunctions();


#endif
