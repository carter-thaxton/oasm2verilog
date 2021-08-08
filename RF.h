
#ifndef RF_H
#define RF_H

#include "SiliconObject.h"

class RF_RAM : public SiliconObject
{
public:
	RF_RAM(const char *name = NULL, Module *parent = NULL);
	virtual ~RF_RAM();

	// Provide definition on instances as well as via a static method
	virtual const SiliconObjectDefinition *Definition() const;
	static  const SiliconObjectDefinition *BuiltinDefinition();

	// Uses GenerateVerilog from SiliconObject

protected:

	// Hook for extra connections during ResolveConnections, before flattening.
	// When wr_width is 1, RF_RAM hooks up the wr_data1 ports identically to the wr_data0 ports
	virtual bool ExtraResolveConnections();

	// Singleton instance for built-in special definition, created on-demand
	static SiliconObjectDefinition *definition;
};


#endif
