
#include "Variable.h"

Variable::Variable(const char *name)
	: Symbol(name), Value(Expression::Unknown())
{
}

Variable::~Variable()
{
	Value.Delete();
}

void Variable::Print(FILE *f) const
{
	fprintf(f, "%s = ", Name());
	Value.Print(f);
}
