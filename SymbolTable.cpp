
#include "SymbolTable.h"
#include "Variable.h"
#include "Parameter.h"
#include "Common.h"


SymbolTable::SymbolTable(bool builtin)
	: builtin(builtin), outerScope(NULL)
{
}

SymbolTable::~SymbolTable()
{
	outerScope = NULL;

	// Delete all variable references and parameter definitions in symbol table
	int n = Count();
	for (int i=0; i < n; i++)
	{
		Symbol *s = Get(i);
		if (s->SymbolType() == SYMBOL_VARIABLE)
		{
			Variable *var = (Variable *) s;
			delete var;
		}
		else if (s->SymbolType() == SYMBOL_PARAMETER)
		{
			ParameterDefinition *param = (ParameterDefinition *) s;
			delete param;
		}
	}
}

Symbol *SymbolTable::Add(Symbol *value)
{
	if (value == NULL) return NULL;

	Symbol *existing = (Symbol *) table.Get(value->Key());
	if (existing)
	{
		// Symbol with same name already found in current scope
		return NULL;
	}

	// Look in outer scopes, to see if an unshadowable symbol exists with the same name
	if (outerScope)
	{
		existing = outerScope->Get(value->Key());
		if (existing && !existing->Shadowable())
		{
			// Found an unshadowable symbol in an outer scope
			return NULL;
		}
	}

	// Add to table
	table.Add(value->Key(), value);
	return value;
}

Symbol *SymbolTable::Get(const char *key) const
{
	if (key == NULL) return NULL;

	Symbol *result = (Symbol *) table.Get(key);

	// Return result if found
	if (result)
		return result;

	if (outerScope)
		// Lookup in outer scope
		return outerScope->Get(key);
	else
		// Simply not found
		return NULL;
}

Symbol *SymbolTable::Get(int i) const
{
	return (Symbol *) table.Get(i);
}

int SymbolTable::Count() const
{
	return table.Count();
}

Variable *SymbolTable::AddVariable(const char *name, const Expression &expr)
{
	Variable *var = new Variable(name);
	var->Value = expr;

	// Try adding Variable to SymbolTable    
	Variable *result = (Variable *) Add(var);

	// If successfully added, add reference to Variable.
	// All Variables in SymbolTable will then be deleted when the SymbolTable is deleted.
	if (result)
	{
		result->Value.IncRef();
	}

	return result;
}

bool SymbolTable::IsBuiltin() const
{
	return builtin;
}


// Manage symbol table chain
SymbolTable *SymbolTable::PushScope(SymbolTable *localScope)
{
	if (localScope->outerScope)
	{
		yyerror("Cannot nest object symbol tables");
		return NULL;
	}

	localScope->outerScope = this;
	return localScope;
}

SymbolTable *SymbolTable::PopScope()
{
	// PopOuter also performs destruction of the current object, if not builtin
	SymbolTable *newLocalScope = this->outerScope;
	if (!builtin)
		delete this;

	// Step over built-in symbol tables, but clear their outerScope pointers
	while (newLocalScope != NULL && newLocalScope->IsBuiltin())
	{
		SymbolTable *tmp = newLocalScope->outerScope;
		newLocalScope->outerScope = NULL;
		newLocalScope = tmp;
	}

	return newLocalScope;
}


// Print out symbol table
void SymbolTable::Print(FILE *f) const
{
	int n = Count();
	fprintf(f, "SymbolTable (%d symbols)\n", n);
	for (int i=0; i < n; i++)
	{
		Symbol *s = Get(i);
		fprintf(f, "\t%s\t", s->Key());
		s->Print(f);
		fprintf(f, "\n");
	}

	// Recursively walk up scope
	if (outerScope)
	{
		fprintf(f, "Outer ");
		outerScope->Print(f);
	}
}
