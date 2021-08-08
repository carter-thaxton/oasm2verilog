
#include "BuiltinFunction.h"
#include "parser.h"
#include <math.h>


// Useful typedefs for functions of various numbers of integer arguments
typedef int (*ZeroArgFcn)  ();
typedef int (*OneArgFcn)   (int);
typedef int (*TwoArgFcn)   (int,int);
typedef int (*ThreeArgFcn) (int,int,int);
typedef int (*FourArgFcn)  (int,int,int,int);


// BuiltinIntegerFunction

BuiltinIntegerFunction::BuiltinIntegerFunction(const char *name, const char *args, void *fcn)
	: Function(name, args), fcn(fcn)
{
}


Expression BuiltinIntegerFunction::Evaluate() const
{
	if (fcn != NULL && NumArgs() == 1)
	{
		ZeroArgFcn fcn0 = (ZeroArgFcn) fcn;
		int result = fcn0();
		return Expression::FromInt(result);
	}
	else
	{
		return Expression::Unknown();
	}
}

Expression BuiltinIntegerFunction::Evaluate(const Expression &op1) const
{
	if (fcn != NULL && NumArgs() == 1 && op1.type == CONST_INT)
	{
		OneArgFcn fcn1 = (OneArgFcn) fcn;
		int result = fcn1(op1.val.i);
		return Expression::FromInt(result);
	}
	else
	{
		return Expression::Unknown();
	}
}

Expression BuiltinIntegerFunction::Evaluate(const Expression &op1, const Expression &op2) const
{
	if (fcn != NULL && NumArgs() == 2 && op1.type == CONST_INT && op2.type == CONST_INT)
	{
		TwoArgFcn fcn2 = (TwoArgFcn) fcn;
		int result = fcn2(op1.val.i, op2.val.i);
		return Expression::FromInt(result);
	}
	else
	{
		return Expression::Unknown();
	}
}

Expression BuiltinIntegerFunction::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const
{
	if (fcn != NULL && NumArgs() == 3 && op1.type == CONST_INT && op2.type == CONST_INT && op3.type == CONST_INT)
	{
		ThreeArgFcn fcn3 = (ThreeArgFcn) fcn;
		int result = fcn3(op1.val.i, op2.val.i, op3.val.i);
		return Expression::FromInt(result);
	}
	else
	{
		return Expression::Unknown();
	}
}

Expression BuiltinIntegerFunction::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const
{
	if (fcn != NULL && NumArgs() == 4 && op1.type == CONST_INT && op2.type == CONST_INT && op3.type == CONST_INT && op4.type == CONST_INT)
	{
		FourArgFcn fcn4 = (FourArgFcn) fcn;
		int result = fcn4(op1.val.i, op2.val.i, op3.val.i, op4.val.i);
		return Expression::FromInt(result);
	}
	else
	{
		return Expression::Unknown();
	}
}





/*
 * Built-in functions available to operate at compile-time
 */
static int int_sqrt(int x)
{
	double d = sqrt((double) x);
	return (int) floor(d);
}

static int int_log2(int x)
{
	double d = log2((double) x);
	return (int) ceil(d);
}

static int int_pow(int base, int exp)
{
	double d = pow(base, exp);
	return (int) floor(d);
}

static int int_exp2(int x)
{
	return 1 << x;
}



// Returns the length of the expression
BuiltinLengthFunction::BuiltinLengthFunction()
	: Function("length", "x")
{
}

Expression BuiltinLengthFunction::Evaluate(const Expression &op1) const
{
	return Expression::FromInt(op1.Length());
}


// Returns a shallow copy of the expression
BuiltinCopyFunction::BuiltinCopyFunction()
	: Function("copy", "x")
{
}

Expression BuiltinCopyFunction::Evaluate(const Expression &op1) const
{
	return op1.Copy();
}



void InitializeBuiltinFunctions()
{
	// Built-in compile-time functions that use all integer arguments
	globalSymbols->Add(new BuiltinIntegerFunction("sqrt", "i", (void*) int_sqrt));
	globalSymbols->Add(new BuiltinIntegerFunction("log2", "i", (void*) int_log2));
	globalSymbols->Add(new BuiltinIntegerFunction("pow",  "ii", (void*) int_pow));
	globalSymbols->Add(new BuiltinIntegerFunction("exp2", "i", (void*) int_exp2));

	// Built-in compile-time functions that directly subclass from Function
	// and accept any expression argument type, not just integer arguments
	globalSymbols->Add(new BuiltinLengthFunction());  // length(x)
	globalSymbols->Add(new BuiltinCopyFunction());    // copy(x)
}


