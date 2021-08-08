
#include "StringMap.h"
#include <string.h>

// Comparison operator for strings in StringMap
bool StringMap_Compare::operator()(char const *a, char const *b) const
{
	return strcmp(a, b) < 0;
}


StringMap::StringMap()
{
}

StringMap::~StringMap()
{
}

int StringMap::Count() const
{
	return map.size();
}

void *StringMap::Add(const char *name, void *value)
{
	if (name == NULL || value == NULL) return NULL;

	StringMapType::const_iterator iter = map.find(name);

	if (iter == map.end())
	{
		// Add to map
		map.insert(StringMapType::value_type(name, value));
		return value;
	}

	// Symbol with same name already found
	return NULL;
}


// Get by name, return NULL if not found
void *StringMap::Get(const char *name) const
{
	if (name == NULL)
		return NULL;

	StringMapType::const_iterator iter = map.find(name);

	// Check if not found
	if (iter == map.end())
	{
		return NULL;
	}

	// Return first found entry
	return iter->second;
}


// Get by index, return NULL if not found
void *StringMap::Get(int i) const
{
	if (i < 0)
		return NULL;

	StringMapType::const_iterator iter;
	for (iter = map.begin(); iter != map.end(); iter++)
	{
		if (i == 0)
			return (*iter).second;
		i--;
	}

	return NULL;
}
