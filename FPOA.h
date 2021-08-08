
#ifndef FPOA_H
#define FPOA_H

#include "SiliconObject.h"

// An FPOA object parses just like a user-defined module, in that it can contain
// instances and inner module definitions.  However, it has a set of parameters
// that correspond to the FPOA_CONTROL object.  In OASM, rather than use an instance
// of the FPOA_CONTROL object in a top-level module, the top-level is simply
// declared as an FPOA with the FPOA_CONTROL parameters.  When it generates Verilog,
// it generates just like a standard module, except that it also instantiates
// an FPOA_CONTROL object, and sets its parameters.

class FPOA : public SiliconObject
{
public:
	FPOA(const char *name = NULL, Module *parent = NULL);
	virtual ~FPOA();

	// Overridden from Module, returns true to indicate top-level FPOA
	virtual bool IsFPOA() const;

	// Perform checks after parse, to ensure that an FPOA object defines a top-level module
	virtual bool AssignResources();

	// Defined in FPOA_GenerateVerilog
	virtual void GenerateVerilog(FILE *f) const;

	// Provide definition on instances as well as via a static method
	virtual const SiliconObjectDefinition *Definition() const;
	static  const SiliconObjectDefinition *BuiltinDefinition();

protected:

	// Singleton instance for built-in special definition, created on-demand
	static SiliconObjectDefinition *definition;
};


#endif
