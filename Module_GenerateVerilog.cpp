
#include "Module.h"
#include "Common.h"

#include <time.h>

//
// Header prepended to every generated file.
// This contains a timestamp and the version of oasm2verilog used to generate the output.
//
void Module::GenerateVerilogHeader(FILE *f)
{
	F0("\n");
	F0("// *** *************************************** ***\n");
	F0("// *** Automatically generated by oasm2verilog ***\n");
	F0("// *** *************************************** ***\n");
	F0("\n");

	time_t now;
	time (&now);
	F1("// Generated at: %s", ctime(&now));

	extern const char *oasm2verilog_version;
	F1("// By: oasm2verilog version %s\n", oasm2verilog_version);
	F0("\n");
	F0("`timescale 1ps / 1ps\n");
	F0("\n");
}


//
// Generates a block of Verilog code to instantiate the delay for a single delayed signal
//
void Module::GenerateVerilogDelay(FILE *f, const Signal *delayedSignal)
{
	// Ensure that the provided argument is in fact a delayed signal
	if (delayedSignal->Behavior != BEHAVIOR_DELAY)
	{
		yyerrorf("Only delayed signals may generate using a DELAY object: '%s'", delayedSignal->Name());
		return;
	}

	const char *delayModuleType;
	switch (delayedSignal->DataType)
	{
		case DATA_TYPE_BIT:     delayModuleType = "DELAY_C";        break;
		case DATA_TYPE_WORD:    delayModuleType = "DELAY_VR";       break;
		default:
			yyerrorf("Unknown data type for signal '%s'.  Cannot generate delay", delayedSignal->Name());
			break;
	}

	// Generate a single line of Verilog that instantiates a delay module, parameterizes its delay,
	// and connects up the input and output ports
	F5("\t%s #(\"%d\") %sD (.i(%s), .o(%s));\n",
		delayModuleType, delayedSignal->DelayCount,
		delayedSignal->Name(),
		delayedSignal->BaseSignal->Name(), delayedSignal->Name());
}


//
// Generates a name-mangled version of the module name
//
void Module::GenerateVerilogModuleName(FILE *f) const
{
	if (ParentModule())
	{
		ParentModule()->GenerateVerilogModuleName(f);
		fprintf(f, "$%s", Name());
	}
	else
	{
		fprintf(f, "%s", Name());
	}
}


//
// Generates wire declarations for all local non-port signals
//
void Module::GenerateVerilogWires(FILE *f) const
{
	// Note: There are no 'reg' data types used in this Verilog notation.
	//       Anonymous constants are skipped, as they are driven as literals directly into the module.
	// wire [16:0] WordReg1;
	// wire        BitWire2;
	// wire        BitReg3;
	// wire [16:0] WordWire4;
	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Direction == DIR_NONE && !sig->Anonymous)
		{
			const char *dt_str = sig->DataType == DATA_TYPE_WORD ? "[16:0]" : "      ";
			F2("\twire   %s %s;\n", dt_str, sig->Name());
		}
	}
}


//
// Generates assignment statements for all connections between local wires
//
void Module::GenerateVerilogConnections(FILE *f) const
{
	bool first = true;
	int n = ConnectionCount();
	for (int i=0; i < n; i++)
	{
		const Connection *con = GetConnection(i);

		// Skip connections to instance ports and built-in signals
		if ((con->Source.ResolvedInstance == NULL) && (con->Destination.ResolvedInstance == NULL) &&
			(con->Source.ResolvedSignal->Behavior != BEHAVIOR_BUILTIN) && (con->Destination.ResolvedSignal->Behavior != BEHAVIOR_BUILTIN))
		{
			// Insert newline before assignments
			if (first)
				F0("\n");
			first = false;

			// assign dest = source;
			F2("\tassign %s = %s;\n", con->Destination.ResolvedSignal->Name(), con->Source.ResolvedSignal->Name());
		}
	}
}


//
// Generates instantiations for all delayed signals in the module.
//
void Module::GenerateVerilogDelays(FILE *f) const
{
	bool first = true;
	int n = SignalCount();
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Behavior == BEHAVIOR_DELAY)
		{
			// Insert newlines before instantiations
			if (first)
				F0("\n\n");
			first = false;

			// Use static utility function to instantiate each delay
			GenerateVerilogDelay(f, sig);
		}
	}
}


//
// Generates instantiations of all instances in the module
//
void Module::GenerateVerilogInstances(FILE *f) const
{
	//
	// Instantiate instances
	//
	bool first = true;

	int ninst = InstanceCount();
	for (int i=0; i < ninst; i++)
	{
		const Instance *inst = GetInstance(i);

		// Skip instances without any definition.  This is an error that should have been caught in a previous pass
		if (inst->Definition)
		{
			F0("\n\t");
			inst->Definition->GenerateVerilogModuleName(f);
			F1(" %s", inst->Name());

			// for now, modules have no parameters

			// create connections
			first = true;

			int ncon = inst->ConnectionCount();
			for (int j=0; j < ncon; j++)
			{
				const Connection *con = inst->GetConnection(j);

				if (first)
					F0("\n\t(\n");
				else
					F0(",\n");
				first = false;

				if (con->Source.ResolvedInstance == inst)
				{
					// output connection from instance
					F2("\t\t.%s(%s)", con->Source.ResolvedSignal->Name(), con->Destination.ResolvedSignal->Name());
				}
				else if (con->Destination.ResolvedInstance == inst)
				{
					// input connection to instance
					F2("\t\t.%s(%s)", con->Destination.ResolvedSignal->Name(), con->Source.ResolvedSignal->Name());
				}

			}

			// end connections
			if (first)
				F0(";\n");
			else
				F0("\n\t);\n");
		}
	}
}

void Module::GenerateVerilog(FILE *f) const
{
	// Generates structural Verilog code to instantiate submodule instances and wire up connections.
	// SiliconObject overrides this method to create a wrapper module around a silicon object instance
	// and drive parameter values.
	// Many silicon objects such as the ALU or MAC further override the method in SiliconObject to
	// match their specific programming models.

	F0("\n");

	F0("//\n");
	F2("// ================ %s - %s ================\n", (ParentModule()) ? "InnerModule" : "Module", Name());
	F0("//\n");

	int n = SignalCount();

	// module ModuleName (
	//     input  [16:0] InWord1,
	//     input         InBit2,
	//     output [16:0] OutWord3
	// );
	F0("module ");
	GenerateVerilogModuleName(f);
	F0(" (");

	bool first = true;
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Direction == DIR_IN || sig->Direction == DIR_OUT)
		{
			if (first)  F0("\n");
			else        F0(",\n");
			first = false;

			const char *dt_str = sig->DataType == DATA_TYPE_WORD ? "[16:0]" : "      ";
			const char *dir_str = sig->Direction == DIR_IN ? "input " : "output";
			F3("\t%s %s %s", dir_str, dt_str, sig->Name());
		}
	}
	F0("\n);\n");


	// Instantiate local wires, connections, delays, and other instances
	GenerateVerilogWires(f);
	GenerateVerilogConnections(f);
	GenerateVerilogDelays(f);
	GenerateVerilogInstances(f);


	// endmodule
	F0("\nendmodule\n");
	F0("\n");


	//
	// Recursively generate inner modules
	//
	int nmodules = InnerModuleCount();
	for (int i=0; i < nmodules; i++)
	{
		const Module *innerModule = GetInnerModule(i);
		if (innerModule)
		{
			innerModule->GenerateVerilog(f);
		}
	}
}


//
// Generates an embedded comment containing an extern module interface declaration
//
void Module::GenerateVerilogEmbeddedExtern(FILE *f, bool suppressEmbeddedOasmDeclarations) const
{
	int n = SignalCount();

	if (!suppressEmbeddedOasmDeclarations)
		F0("//+++EMBEDDED_OASM+++\n");

	// Declare all object types as "module", except top-level "FPOA"
	const char *moduleType = IsFPOA() ? "FPOA" : "module";


	F2("// extern %s %s\n", moduleType, Name());
	F0("// {\n");
	for (int i=0; i < n; i++)
	{
		const Signal *sig = GetSignal(i);
		if (sig->Behavior != BEHAVIOR_BUILTIN && sig->Direction != DIR_NONE)
		{
			const char *data_type_str = (sig->DataType == DATA_TYPE_WORD) ? "word" : "bit ";
			const char *dir_str = (sig->Direction == DIR_OUT) ? "output" : "input ";

			F3("//     %s %s %s;\n", dir_str, data_type_str, sig->Name());
		}
	}
	F0("// }\n");

	if (!suppressEmbeddedOasmDeclarations)
		F0("//+++END_EMBEDDED_OASM+++\n");
}