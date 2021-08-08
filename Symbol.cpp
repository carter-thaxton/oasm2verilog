
#include "Symbol.h"

Symbol::Symbol(const char *name)
{
	this->name = name;
}

Symbol::~Symbol()
{
}

const char *Symbol::Name() const
{
	return this->name;
}

// By default Key is Name, but some classes override this to provide unique mangled names in the symbol table
const char *Symbol::Key() const
{
	return Name();
}

void Symbol::Print(FILE *f) const
{
	fprintf(f, "%s", name);
}
