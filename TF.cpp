
#include "TF.h"
#include "Common.h"

/*
 * TF_Module
 */
TF_Module::TF_Module(const char *name, Module *parent)
	: SiliconObject(name, parent),
	  tf_regs(), tf_logic(), num_tfs(0)
{
}

TF_Module::~TF_Module()
{
}

void TF_Module::Print(FILE *f) const
{
	Module::Print(f);

	if (num_tfs > 0)
	{
		fprintf(f, "\tTF\n\t{\n");
		for (int i=0; i < num_tfs; i++)
		{
			fprintf(f, "\t\t%s = ", tf_regs[i]->Name());
			tf_logic[i].Print(f);
			fprintf(f, "\n");
		}
		fprintf(f, "\t}\n");
	}
}

// Resource allocation is performed as TFs are added
Signal *TF_Module::AddTF(Signal *dest, const TruthFunction &tf)
{
	// Check whether dest is declared
	bool found = false;
	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		if (GetSignal(i) == dest)
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		yyerrorf("TF bit register '%s' not declared", dest->Name());
		return NULL;
	}

	// Check behavior and data type
	if (dest->Behavior != BEHAVIOR_REG || dest->DataType != DATA_TYPE_BIT)
	{
		yyerrorf("TF register '%s' must be declared as a bit reg", dest->Name());
		return NULL;
	}

	// Check whether already assigned
	if (dest->RegisterNumber >= 0)
	{
		yyerrorf("TF register '%s' has already been assigned", dest->Name());
		return NULL;
	}

	// Check whether too many TFs have already been assigned
	if (num_tfs >= MAX_TFS)
	{
		yyerrorf("Too many bit registers in ALU.  Only %d TFs are allowed", MAX_TFS);
		return NULL;
	}

	// Assign the TF register and associated TF logic
	tf_regs[num_tfs] = dest;
	tf_logic[num_tfs] = tf;
	dest->RegisterNumber = num_tfs;
	num_tfs++;

	return dest;
}



/*
 * FloatingTF
 */
FloatingTF::FloatingTF(const char *name, Module *parent)
	: TF_Module(name, parent)
{
}

FloatingTF::~FloatingTF()
{
}


// Assign bit signals to tf_regs
// Generate errors as appropriate
// Returns true if successful
bool FloatingTF::AssignResources()
{
	bool ok = true;

	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		Signal *sig = GetSignal(i);

		// Only assign unassigned registers
		if (sig->RegisterNumber < 0)
		{
			if (sig->Behavior == BEHAVIOR_REG || sig->Behavior == BEHAVIOR_CONST)
			{
				if (sig->DataType == DATA_TYPE_WORD)
				{
					yyerrorfl(Location, "TFs can only declare bit registers.  '%s' is declared as a word", sig->Name());
					ok = false;
					continue;
				}
				else if (sig->DataType == DATA_TYPE_BIT)
				{
					if (num_tfs >= MAX_TFS)
					{
						yyerrorfl(Location, "Too many bit registers in TF.  Only %d TFs are allowed", MAX_TFS);
						ok = false;
						continue;
					}

					// Unassigned TFs should hold their previous value
					tf_regs[num_tfs] = sig;
					tf_logic[num_tfs] = TruthFunction::FromSignal(sig);
					sig->RegisterNumber = num_tfs;
					num_tfs++;
				}
			}
		}
	}

	if (!ok)
		return false;

	return Module::AssignResources();
}


/*
 * Singleton instance of definition, created on-demand
 */

// Factory method to create TF
static SiliconObject *TF_Factory(const SiliconObjectDefinition *definition, const char *name, Module *parent)
{
	return new FloatingTF(name, parent);
}


const SiliconObjectDefinition *FloatingTF::Definition() const
{
	// Use built-in static method
	return BuiltinDefinition();
}

const SiliconObjectDefinition *FloatingTF::BuiltinDefinition()
{
	// Return singleton instance if already created
	if (definition)
		return definition;

	// Create a new singleton definition
	// No built-in symbol table needed for TF
	definition = new SiliconObjectDefinition("TF", NULL, TF_Factory);

	return definition;
}

SiliconObjectDefinition *FloatingTF::definition = NULL;
