
#ifndef STRING_MAP_H
#define STRING_MAP_H

#include <map>
using namespace std;

// Special class used for comparison of strings
struct StringMap_Compare
{
	bool operator()(char const *a, char const *b) const;
};

// Typedef for underlying map that uses StringMap_Compare
typedef map<const char*,void*,StringMap_Compare> StringMapType;


class StringMap
{
public:
	StringMap();
	virtual ~StringMap();

	// Return number of items
	int Count() const;

	// Add and lookup values
	void *Add(const char *name, void *value);       // Add by name
	void *Get(const char *name) const;              // Get by name
	void *Get(int i) const;                         // Get by index

private:
	StringMapType map;
};


#endif
