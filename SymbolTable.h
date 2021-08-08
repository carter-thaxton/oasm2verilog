
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Symbol.h"
#include "StringMap.h"
#include "Variable.h"
#include "Expression.h"
#include <stdio.h>

class SymbolTable
{
public:
	SymbolTable(bool builtin = false);
	virtual ~SymbolTable();

	// Add and lookup symbols
	Symbol *Add(Symbol *value);
	Symbol *Get(const char *key) const;
	Symbol *Get(int i) const;

	// Get number of symbols in current scope
	int Count() const;

	// Some symbol tables are built-in, and should not be deleted
	bool IsBuiltin() const;

	// Modifies the localScope to point to this symbol table, and returns the local symbol table
	SymbolTable *PushScope(SymbolTable *localScope);

	// Deletes this symbol table and returns the outer scope (avoids deleting builtin symbol tables)
	SymbolTable *PopScope();

	// Special calls for known types of Symbol
	Variable *AddVariable(const char *name, const Expression &expr);

	// Print for debug
	void Print(FILE *f) const;

private:
	bool builtin;
	SymbolTable *outerScope;
	StringMap table;
};


#endif
