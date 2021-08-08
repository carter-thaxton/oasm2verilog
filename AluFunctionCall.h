
#ifndef ALU_FUNCTION_CALL_H
#define ALU_FUNCTION_CALL_H

class Signal;
class AluFunction;

union AluFunctionArg
{
	Signal *sig;
	int i;
};

struct AluFunctionCall
{
	AluFunction *Fcn;
	AluFunctionArg Args[4];
};

#endif
