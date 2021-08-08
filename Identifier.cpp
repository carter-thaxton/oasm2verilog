
#include "Identifier.h"
#include "Common.h"

// Initialization routine for DottedIdentifer, in lieu of a constructor
void DottedIdentifier::Init()
{
	Name = NULL;
	Next = NULL;
}

// Cleanup routine for DottedIdentifer
void DottedIdentifier::Delete()
{
	DottedIdentifier *id = Next;
	while (id)
	{
		DottedIdentifier *temp = id;
		id = id->Next;
		delete temp;  temp = NULL;
	}
	Name = NULL;
	Next = NULL;
}

// Returns the number of names in this identifier
int DottedIdentifier::Depth() const
{
	if (Name == NULL)
		return 0;

	if (Next)
		return 1 + Next->Depth();
	else
		return 1;
}

// Prints as a dotted identifier!
void DottedIdentifier::Print(FILE *f) const
{
	fprintf(f, "%s", Name);

	if (Next)
	{
		fprintf(f, ".");
		Next->Print(f);
	}
}


// In order to match, the depth must be the same, and all strings must match
bool DottedIdentifier::operator==(const DottedIdentifier &id) const
{
	// No null strings supported
	if (Name == NULL || id.Name == NULL) return false;

	// Not the same names
	if (strcmp(Name, id.Name) != 0) return false;

	// If we get here, we have reached the end and the identifiers match
	if (!Next && !id.Next)
		return true;

	// Keep searching recursively
	if (Next && id.Next)
		return (*Next == *id.Next);

	// Not the same length
	return false;
}

bool DottedIdentifier::operator!=(const DottedIdentifier &id) const
{
	// Invert result of operator==
	return !(*this == id);
}


// Creates a temporary string in buf, up to buflen characters
const char *DottedIdentifier::ToString(char *buf, int buflen) const
{
	*buf = 0;
	int len = strcatbuf(buf, buflen, Name);
	if (Next)
	{
		len = strcatbuf(buf, buflen, ".");
		Next->ToString(&buf[len], buflen-len);
	}

	return buf;
}




// Initialization routine for DottedIdentiferList, in lieu of a constructor
void DottedIdentifierList::Init()
{
	Id.Init();
	Next = NULL;
}


// Cleanup routine for DottedIdentiferList
// Deletes the DottedIdentiferLists, but keeps the underlying DottedIdentifiers
void DottedIdentifierList::Delete()
{
	DottedIdentifierList *list = Next;
	while (list)
	{
		DottedIdentifierList *temp = list;
		list = list->Next;
		delete temp;  temp = NULL;
	}

	Next = NULL;
}


// Cleanup routine for DottedIdentiferList
// Deletes both the DottedIdentiferLists and the DottedIdentifiers
void DottedIdentifierList::DeleteAll()
{
	DottedIdentifierList *list = Next;
	while (list)
	{
		DottedIdentifierList *temp = list;
		list = list->Next;
		temp->Id.Delete();
		delete temp;  temp = NULL;
	}

	Id.Delete();
	Next = NULL;
}

// Returns the number of DottedIdentifiers in this list
int DottedIdentifierList::Count() const
{
	if (Next)
		return 1 + Next->Count();
	else
		return 1;
}
