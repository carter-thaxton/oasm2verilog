
#include "FPOA.h"
#include "Common.h"


void FPOA::GenerateVerilog(FILE *f) const
{
	// Identical to Module::GenerateVerilog, except that it also generates an FPOA_CONTROL object

	F0("\n");

	F0("//\n");
	F2("// ================ %s - %s ================\n", (ParentModule()) ? "InnerModule" : "Module", Name());
	F0("//\n");

	// module ModuleName (
	// );
	F0("module ");
	GenerateVerilogModuleName(f);
	F0(" (\n");
	F0(");\n");

	// Instantiate built-in signals
	F0("\twire          core_clock;\n");


	// Instantiate local wires, connections, delays, and other instances
	GenerateVerilogWires(f);
	GenerateVerilogConnections(f);
	GenerateVerilogDelays(f);
	GenerateVerilogInstances(f);


	// Instantiate FPOA_CONTROL object
	F0("\n\tFPOA_CONTROL ");

	// parameters
	bool first = true;

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
	
	F0("FPOA_CONTROL\n");
	F0("\t(\n");

	F0("\t\t.core_clock(core_clock)\n");
	F0("\t);\n");


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

