
#ifndef TF_H
#define TF_H

#define MAX_TFS         (4)

#include "SiliconObject.h"
#include "Module.h"
#include "Signal.h"
#include "SymbolTable.h"
#include "TruthFunction.h"

// Base class for FloatingTF and ALU
class TF_Module : public SiliconObject
{
public:
	TF_Module(const char *name, Module *parent = NULL);
	virtual ~TF_Module();

	// No definition for this intermediate base class
	virtual const SiliconObjectDefinition *Definition() const = 0;

	virtual void Print(FILE *f) const;

	virtual Signal *AddTF(Signal *dest, const TruthFunction &tf);

protected:
	Signal *tf_regs[MAX_TFS];
	TruthFunction tf_logic[MAX_TFS];
	int num_tfs;
};


// Standalone TF separate from ALU
class FloatingTF : public TF_Module
{
public:
	FloatingTF(const char *name, Module *parent = NULL);
	virtual ~FloatingTF();

	// Provide definition on instances as well as via a static method
	virtual const SiliconObjectDefinition *Definition() const;
	static  const SiliconObjectDefinition *BuiltinDefinition();

	// Defined in TF_GenerateVerilog
	virtual void GenerateVerilog(FILE *f) const;

protected:
	virtual bool AssignResources();

private:
	// Singleton instance for built-in special definition, created on-demand
	static SiliconObjectDefinition *definition;
};


#endif
