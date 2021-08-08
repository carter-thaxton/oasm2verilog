
#include "AluFunction.h"
#include "AluFunctionCall.h"
#include "Common.h"


AluFunction::AluFunction(const char *name, const char *args)
	: Function(name, args)
{
}

AluFunction::~AluFunction()
{
}


void AluFunction::Print(FILE *f) const
{
	fprintf(f, "ALU Inst: %s (%s)\n", name, args);
}


Expression AluFunction::Evaluate() const
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn.Fcn = (AluFunction *) this;
	expr.val.fcn.Args[0].sig = NULL;
	expr.val.fcn.Args[1].sig = NULL;
	expr.val.fcn.Args[2].sig = NULL;
	expr.val.fcn.Args[3].sig = NULL;
	return expr;
}

Expression AluFunction::Evaluate(const Expression &op1) const
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn.Fcn = (AluFunction *) this;

	expr.val.fcn.Args[0] = ToAluFunctionArg(op1, ArgType(0));
	expr.val.fcn.Args[1].sig = NULL;
	expr.val.fcn.Args[2].sig = NULL;
	expr.val.fcn.Args[3].sig = NULL;
	return expr;
}

Expression AluFunction::Evaluate(const Expression &op1, const Expression &op2) const
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn.Fcn = (AluFunction *) this;
	expr.val.fcn.Args[0] = ToAluFunctionArg(op1, ArgType(0));
	expr.val.fcn.Args[1] = ToAluFunctionArg(op2, ArgType(1));
	expr.val.fcn.Args[2].sig = NULL;
	expr.val.fcn.Args[3].sig = NULL;
	return expr;
}

Expression AluFunction::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3) const
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn.Fcn = (AluFunction *) this;
	expr.val.fcn.Args[0] = ToAluFunctionArg(op1, ArgType(0));
	expr.val.fcn.Args[1] = ToAluFunctionArg(op2, ArgType(1));
	expr.val.fcn.Args[2] = ToAluFunctionArg(op3, ArgType(2));
	expr.val.fcn.Args[3].sig = NULL;
	return expr;
}

Expression AluFunction::Evaluate(const Expression &op1, const Expression &op2, const Expression &op3, const Expression &op4) const
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn.Fcn = (AluFunction *) this;
	expr.val.fcn.Args[0] = ToAluFunctionArg(op1, ArgType(0));
	expr.val.fcn.Args[1] = ToAluFunctionArg(op2, ArgType(1));
	expr.val.fcn.Args[2] = ToAluFunctionArg(op3, ArgType(2));
	expr.val.fcn.Args[3] = ToAluFunctionArg(op4, ArgType(3));
	return expr;
}


// static helper function to convert to the appropriate AluFunctionArg type
AluFunctionArg AluFunction::ToAluFunctionArg(const Expression &op, char argType)
{
	AluFunctionArg result;

	if (argType == 'k')
		result.i = op.ToInt();
	else
		result.sig = op.ToSignal();
	
	return result;
}

