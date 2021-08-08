
#ifndef IDENTIFIER_H
#define IDENTIFIER_H

#include "Common.h"


/*
 * Common type for potentially dotted identifiers
 */
struct DottedIdentifier
{
	char *Name;
	DottedIdentifier *Next;

	void Init();		// In lieu of a constructor, because it is used in the yylval union
	void Delete();		// Deletes the underlying list of DottedIdentfiers

	int Depth() const;	// Returns the size of the list, which is 1 plus the number of dots in the original identifier

	void Print(FILE *f) const;

	// In order to match, the depth must be the same, and all strings must match
	bool operator==(const DottedIdentifier &id) const;
	bool operator!=(const DottedIdentifier &id) const;

	// Creates a temporary string in buf, up to buflen characters
	const char *ToString(char *buf, int buflen) const;
};


/*
 * Represents a list of DottedIdentifiers, used for example in
 * comma-separated lists of destinations during structural connections,
 * or in instruction register destinations.
 */
struct DottedIdentifierList
{
	DottedIdentifier Id;
	DottedIdentifierList *Next;

	void Init();		// In lieu of a constructor, because it is used in the yylval union
	void Delete();		// Deletes the DottedIdentiferLists, but keeps the underlying DottedIdentifiers
	void DeleteAll();	// Deletes both the DottedIdentiferLists and the DottedIdentifiers

	int Count() const;	// Returns the number of DottedIdentifers in the list
};

#endif
