
#include "EnumValue.h"

EnumValue::EnumValue(const char *name)
	: Symbol(name)
{
}

EnumValue::~EnumValue()
{
}

void EnumValue::Print(FILE *f) const
{
	fprintf(f, "%s", Name());
}

Expression EnumValue::ToExpression() const
{
	return Expression::FromEnum(Name());
}
