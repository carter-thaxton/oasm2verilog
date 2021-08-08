
#include "SiliconObject.h"

void SiliconObject::GenerateVerilog(FILE *f) const
{
	// This version of GenerateVerilog is common across all silicon object types.
	// Many module types will override this, such as the ALU or MAC, because they have
	// specific programming models.

	// This version simply generates parameters and structural code for defined built-in ports.
	// It is illegal to instantiate submodules inside of SiliconObjects, so that is not present here.

	F0("\n");

	F0("//\n");
	F2("// ================ %s - %s ================\n", ModuleTypeName(), Name());
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
		Signal *sig = GetSignal(i);
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


	// Instantiate local wires, connections, and delays
	GenerateVerilogWires(f);
	GenerateVerilogConnections(f);
	GenerateVerilogDelays(f);


	// <module_type_name>
	F1("\n\t%s ", ModuleTypeName());
	// Note: #( is generated on demand

	// parameters
	first = true;

	int nparams = ParameterCount();
	for (int i=0; i < nparams; i++)
	{
		const Parameter *param = GetParameter(i);

		if (first)
			F0("\n\t#(\n");
		else
			F0(",\n");
		first = false;

		param->GenerateVerilog(f);
	}


	// end parameters
	if (!first)
		F0("\n\t)\n\t");
	
	// ModuleName
	// (
	F1("%s", Name());

	// signals
	first = true;

	int ncon = ConnectionCount();
	for (int i=0; i < ncon; i++)
	{
		const Connection *con = GetConnection(i);

		if (con->Destination.ResolvedSignal->Behavior == BEHAVIOR_BUILTIN)
		{
			// Input to a built-in signal
			if (first)
				F0("\n\t(\n");
			else
				F0(",\n");
			first = false;

			F2("\t\t.%s(%s)", con->Destination.ResolvedSignal->Name(), con->Source.ResolvedSignal->Name());
		}
		else if (con->Source.ResolvedSignal->Behavior == BEHAVIOR_BUILTIN)
		{
			// Output from a built-in signal
			if (first)
				F0("\n\t(\n");
			else
				F0(",\n");
			first = false;

			F2("\t\t.%s(%s)", con->Source.ResolvedSignal->Name(), con->Destination.ResolvedSignal->Name());
		}
	}


	// end signals
	if (first)
		F0(";\n\n");
	else
		F0("\n\t);\n\n");

	// endmodule
	F0("endmodule\n");
	F0("\n");
}
