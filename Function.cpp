
#include "Function.h"
#include "SymbolTable.h"
#include "Common.h"
#include "Signal.h"


// Function

Function::Function(const char *name, const char *args)
	: Symbol(name), args(args)
{
	// Initialize function key in SymbolTable to "name__nargs", e.g. "log2__1"
	int nargs = 0;
	if (args)
		nargs = strlen(args);

	key = (char *) malloc(strlen(name) + 8);
	sprintf(key, "%s__%d", name, nargs);
}

Function::~Function()
{
	free((void*) key);
}

int Function::NumArgs() const
{
	if (args == NULL)
		return 0;

	return strlen(args);
}

char Function::ArgType(int arg) const
{
	if (arg < 0 || arg >= NumArgs())
		return 0;

	return args[arg];
}

const char *Function::Key() const
{
	return key;
}

void Function::Print(FILE *f) const
{
	fprintf(f, "Function: %s (%s)\n", name, args);
}


// Undefined stubs for Evaluate
Expression Function::Evaluate() const
{
	yyerrorf("Function '%s' undefined with 0 arguments", Name());
	return Expression::Unknown();
}

Expression Function::Evaluate(const Expression &op1) const
{
	yyerrorf("Function '%s' undefined with 1 argument", Name());
	return Expression::Unknown();
}

Expression Function::Evaluate(const Expression &op1, const Expression &op2) const
{
	yyerrorf("Function '%s' undefined with 2 arguments", Name());
	return Expression::Unknown();
}

Expression Function::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const
{
	yyerrorf("Function '%s' undefined with 3 arguments", Name());
	return Expression::Unknown();
}

Expression Function::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const
{
	yyerrorf("Function '%s' undefined with 4 arguments", Name());
	return Expression::Unknown();
}





// Static helper function to call a BuiltinFunction or create an AluFunctionCall by name, given the expression arguments, and using the local symbol table
Expression Function::Call(SymbolTable *symbols, const char *fcn_name, int nargs, const Expression *op1, const Expression *op2, const Expression *op3, const Expression *op4)
{
	// Lookup function in SymbolTable, by using a temporary key constructed from the name and number of arguments
	char *fcn_key = (char *) malloc(strlen(fcn_name) + 8);
	sprintf(fcn_key, "%s__%d", fcn_name, nargs);

	Symbol *symbol = symbols->Get(fcn_key);

	// Free created key
	free(fcn_key);

	if (!symbol)
	{
		yyerrorf("Function '%s' not defined with %d arguments", fcn_name, nargs);
		return Expression::Unknown();
	}
	else if (symbol->SymbolType() != SYMBOL_FUNCTION && symbol->SymbolType() != SYMBOL_ALU_INST)
	{
		yyerrorf("Attempt to call non-function symbol '%s' as a function", fcn_name);
		fprintf(stderr, "symbolType: %d\n", symbol->SymbolType());

		return Expression::Unknown();
	}
	else
	{
		Function *fcn = (Function *) symbol;

		// Check number of arguments
		if (fcn->NumArgs() != nargs)
		{
			yyerrorf("Function '%s' expects %d arguments, but is called with %d", fcn_name, fcn->NumArgs(), nargs);
			return Expression::Unknown();
		}

		// Check argument types
		const Expression *ops[4] = {op1, op2, op3, op4};
		for (int i=0; i < nargs; i++)
		{
			if (ops[i] == NULL)
			{
				yyerrorf("Function called with %d arguments, but argument %d is NULL", nargs, i);
				return Expression::Unknown();
			}

			// Check argument expression according to args type string
			if (!fcn->CheckArgumentType(i, *ops[i]))
				return Expression::Unknown();
		}


		// Dispatch call to appropriate evaulation method
		if (nargs == 0)
		{
			return fcn->Evaluate();
		}
		else if (nargs == 1)
		{
			return fcn->Evaluate(*op1);
		}
		else if (nargs == 2)
		{
			return fcn->Evaluate(*op1, *op2);
		}
		else if (nargs == 3)
		{
			return fcn->Evaluate(*op1, *op2, *op3);
		}
		else if (nargs == 4)
		{
			return fcn->Evaluate(*op1, *op2, *op3, *op4);
		}
		else
		{
			return Expression::Unknown();
		}
	}
}

// Helper method to determine whether a function accepts a given argument
bool Function::CheckArgumentType(int arg, const Expression &op) const
{
	// Check argument based on type character:
	// x - any expression
	// j - boolean const integer
	// k - small const integer
	// i - const integer
	// s - const string
	// e - const enum
	// w - word
	// b - bit

	if (arg < 0)
		return false;
	if (arg >= NumArgs())
		return false;

	char fcn_type = args[arg];
	switch (fcn_type)
	{
		// Accept any expression
		case 'x':
			return true;

		// Accept a constant boolean (integer 0-1)
		case 'j':
			if (op.type == CONST_INT)
			{
				if (op.val.i >= 0 && op.val.i <= 1)
					return true;

				yyerrorf("Function '%s' provided out of range value %d in argument %d.  Expects a bit", name, op.val.i, arg);
				return false;
			}
			break;

		// Accept a constant integer from 0-15
		case 'k':
			if (op.type == CONST_INT)
			{
				if (op.val.i >= 0 && op.val.i <= 15)
					return true;

				yyerrorf("Function '%s' provided out of range value %d in argument %d.  Expects an integer from 0 to 15", name, op.val.i, arg);
				return false;
			}
			break;

		// Accept any constant integer
		case 'i':
			if (op.type == CONST_INT)
			{
				return true;
			}
			break;

		// Accept a constant string
		case 's':
			if (op.type == CONST_STRING)
			{
				return true;
			}
			break;
			
		// Accept a constant enum
		case 'e':
			if (op.type == CONST_ENUM)
			{
				return true;
			}
			break;

		// Accept a word signal or 16-bit constant integer
		case 'w':
			if (op.type == EXPRESSION_SIGNAL)
			{
				Signal *sig = op.val.sig;
				if (sig->DataType == DATA_TYPE_WORD)
					return true;

				else if (sig->DataType == DATA_TYPE_BIT)
				{
					yyerrorf("Function '%s' provided a bit in argument %d.  Expects a word", name, arg);
					return false;
				}
			}
			else if (op.type == CONST_INT)
			{
				if (op.val.i >= -32768 && op.val.i <= 65535)
					return true;

				yyerrorf("Function '%s' provided out of range value 0x%x in argument %d.  Expects a 16-bit integer", name, op.val.i, arg);
				return false;
			}
			break;

		// Accept a bit signal or constant integer 0-1
		case 'b':
			if (op.type == EXPRESSION_SIGNAL)
			{
				Signal *sig = op.val.sig;
				if (sig->DataType == DATA_TYPE_BIT)
					return true;

				else if (sig->DataType == DATA_TYPE_WORD)
				{
					yyerrorf("Function '%s' provided a word in argument %d.  Expects a bit", name, arg);
					return false;
				}
			}
			else if (op.type == CONST_INT)
			{
				if (op.val.i >= 0 && op.val.i <= 1)
					return true;

				yyerrorf("Function '%s' provided out of range value %d in argument %d.  Expects a bit", name, op.val.i, arg);
				return false;
			}
			break;
	}

	yyerrorf("Function '%s' called with illegal type in argument %d.  Expects type %s", name, arg, TypeStringFromChar(fcn_type));
	return false;
}

// static helper method to return a human-readable string from a type character
const char *Function::TypeStringFromChar(char c)
{
	switch (c)
	{
		case 'x':  return "any";
		case 'j':  return "boolean";
		case 'k':  return "integer (0-15)";
		case 'i':  return "integer";
		case 's':  return "string";
		case 'w':  return "word";
		case 'b':  return "bit";
	}
	return NULL;
}
