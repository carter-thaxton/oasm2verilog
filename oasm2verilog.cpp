
/*
 *  =======  oasm2verilog  =======
 */



#include <stdio.h>
#include <string.h>
#include <vector>
using namespace::std;

#include "parser.h"


// Version Information
const char *oasm2verilog_version = "0.3";

// Command-Line Parameters
vector<const char *> input_filenames;
vector<ParseMode> input_file_modes;

char *output_filename = NULL;
FILE *output_file = NULL;

bool printHelp = false;
bool printVersion = false;
bool noEmbeddedOasm = false;
bool parseOnly = false;
bool generateReport = false;
bool warnAsError = false;

void Usage(FILE *f)
{
	fprintf(f, "oasm2verilog [options] [-o <verilog_file>] [-l <library_file>] <oasm_file1> [...]\n");
	fprintf(f, "  -h                Help                 (this message)\n");
	fprintf(f, "  -v                Version              (current version %s)\n", oasm2verilog_version);
	fprintf(f, "  -o [out_file]     Output Verilog file  (defaults to stdout)\n");
	fprintf(f, "  -l [in_file]      Read library file containing embedded OASM\n");
	fprintf(f, "  -n                Do not generate embedded OASM section in Verilog output\n");
	fprintf(f, "  -p                Parse only and report errors.  Do not generate Verilog\n");
	fprintf(f, "  -r                Generate report after parsing\n");
	fprintf(f, "  -w                Warnings become errors\n");
	fprintf(f, "  --debug           Enable debug mode\n");
}

void PrintVersion(FILE *f)
{
	fprintf(f, "oasm2verilog version %s\n", oasm2verilog_version);
}

int ParseCommandLine(int argc, char *argv[])
{
	int i = 1;
	while (i < argc)
	{
		char *arg = argv[i];
		
		// Help
		if ((strcmp(arg, "-h") == 0) || (strcmp(arg, "--h") == 0) || (strcmp(arg, "-help") == 0) || (strcmp(arg, "--help") == 0))
		{
			printHelp = true;
		}

		// Version
		else if ((strcmp(arg, "-v") == 0) || (strcmp(arg, "--v") == 0) || (strcmp(arg, "-version") == 0) || (strcmp(arg, "--version") == 0))
		{
			printVersion = true;
		}

		// No embedded OASM in output Verilog
		else if (strcmp(arg, "-n") == 0)
		{
			noEmbeddedOasm = true;
		}

		// Parse only
		else if (strcmp(arg, "-p") == 0)
		{
			parseOnly = true;
		}

		// Generate report
		else if (strcmp(arg, "-r") == 0)
		{
			generateReport = true;
		}

		// Warn as errors
		else if (strcmp(arg, "-w") == 0)
		{
			warnAsError = true;
		}

		// Enable debugging
		else if (strcmp(arg, "--debug") == 0)
		{
			yydebug = true;
		}

		// Output Verilog file
		else if (strcmp(arg, "-o") == 0)
		{
			i++;
			if (i >= argc) return 0;
			if (output_file) return 0;
			output_filename = argv[i];
		}

		// Embedded OASM file
		else if (strcmp(arg, "-l") == 0)
		{
			i++;
			if (i >= argc) return 0;

			input_filenames.push_back(argv[i]);
			input_file_modes.push_back(PARSE_EMBEDDED_OASM);
		}

		// Unrecognized option
		else if (arg[0] == '-' && arg[1] != 0)
		{
			fprintf(stderr, "Unrecognized option: %s\n", arg);
			return 0;
		}

		// OASM file
		else
		{
			input_filenames.push_back(arg);
			input_file_modes.push_back(PARSE_OASM);
		}
		i++;
	}

	return 1;
}


bool ResolveInstances()
{
	bool ok = true;
	for (int i=0; i < modules.Count(); i++)
	{
		Module *module = (Module *) modules.Get(i);
		if (!module->ResolveInstances())
			ok = false;
	}
	return ok;
}


bool ResolveConnections()
{
	bool ok = true;
	for (int i=0; i < modules.Count(); i++)
	{
		Module *module = (Module *) modules.Get(i);
		if (!module->ResolveConnections())
			ok = false;
	}
	return ok;
}


void DeleteModules()
{
	for (int i=0; i < modules.Count(); i++)
	{
		Module *module = (Module *) modules.Get(i);
		delete module;  module = NULL;
	}
	//FIXME clear out modules collection
}


void GenerateReport(FILE *f)
{
	fprintf(f, "== %d module(s) defined ==\n", modules.Count());
	for (int i=0; i < modules.Count(); i++)
	{
		Module *module = (Module *) modules.Get(i);
		module->Print(f);
	}
	fprintf(f, "== %d module(s) defined ==\n", modules.Count());
}


// Generate Verilog into the output file provided
void GenerateVerilog(FILE *f, bool generateEmbeddedOasmSection)
{
	// Start with a boilerplate header
	Module::GenerateVerilogHeader(f);

	int n = modules.Count();

	// Generate embedded OASM section in hot comments at the top of the file
	// These should only be generated for inner modules
	if (generateEmbeddedOasmSection)
	{
		fprintf(f, "//+++EMBEDDED_OASM+++\n");
		for (int i=0; i < n; i++)
		{
			Module *module = (Module *) modules.Get(i);

			// Skip extern modules
			if (!module->IsExtern())
			{
				module->GenerateVerilogEmbeddedExtern(f, true);
			}
		}
		fprintf(f, "//+++END_EMBEDDED_OASM+++\n");
		fprintf(f, "\n");
	}

	// Generate Verilog for each non-extern top-level module
	// Each module will generate its inner modules
	for (int i=0; i < n; i++)
	{
		Module *module = (Module *) modules.Get(i);

		// Skip extern modules
		if (!module->IsExtern())
		{
			module->GenerateVerilog(f);
		}
	}
}


// Main Entry Point
int main(int argc, char *argv[])
{
	if (!ParseCommandLine(argc, argv))
	{
		Usage(stderr);
		return 1;
	}

	if (printHelp)
	{
		Usage(stdout);
		return 0;
	}

	if (printVersion)
	{
		PrintVersion(stdout);
		return 0;
	}

	if (input_filenames.size() == 0)
	{
		Usage(stderr);
		return 1;
	}


	// Initialize parser
	InitParser();

	bool ok = true;

	// Parse each input file from command-line in turn
	for (int i=0; i < (int) input_filenames.size(); i++)
	{
		const char *input_filename = input_filenames[i];
		ParseMode input_file_mode = input_file_modes[i];

		FILE *input_file;
		if (strcmp(input_filename, "-") == 0)
		{
			// - means use STDIN
			input_file = stdin;
			input_filename = NULL;
		}
		else
		{
			// open filename
			input_file = fopen(input_filename, "r");
			if (!input_file)
			{
				fprintf(stderr, "ERROR - Cannot open input file: %s\n", input_filename);
				ok = false;
				break;
			}
		}

		// Call the parser.  Returns 0 if ok, 1 if a parse error occurred, and 2 if a fatal error occurred
		// Exit immediately if err is 2
		int err = ParseFile(input_file, input_filename, input_file_mode);
		if (err == 2)
			exit(1);

		// Close explicitly opened files
		if (input_filename)
			fclose(input_file);

		// Check for errors and stop parsing other files if an error occurred
		if (errorCount > 0)
		{
			ok = false;
			break;
		}
	}


	// Perform additional passes after parsing all input files
	if (ok)
	{
		ok = ResolveInstances();
	}

	if (ok)
	{
		ok = ResolveConnections();
	}


	// Report warnings
	if (warnCount > 0)
	{
		if (warnCount == 1)
			fprintf(stderr, "%d warning occurred\n", warnCount);
		else
			fprintf(stderr, "%d warnings occurred\n", warnCount);
	}

	// Report errors
	if (errorCount > 0)
	{
		ok = false;
		if (errorCount == 1)
			fprintf(stderr, "%d error occurred\n", errorCount);
		else
			fprintf(stderr, "%d errors occurred\n", errorCount);
	}


	// Generate a simple report of all parsed modules.
	if (generateReport)
	{
		GenerateReport(stdout);
	}

	if (yydebug)
	{
		// Print out some additional reference counts, for debugging
		printf("ExpressionArray::TotalRefCount after parse:  %d\n", ExpressionArray::TotalRefCount);
	}

	// Produce Verilog output if no errors, and if parseOnly (-p) is not set
	if (ok && !parseOnly)
	{
		if (!output_filename)
		{
			output_file = stdout;
		}
		else
		{
			output_file = fopen(output_filename, "w");
			if (!output_file)
			{
				fprintf(stderr, "ERROR - Cannot open output file: %s\n", output_filename);
				return 1;
			}
		}

		// When -n switch is set, do not generate embedded OASM into Verilog
		GenerateVerilog(output_file, !noEmbeddedOasm);
	}

	// Clean up all module data structures
	DeleteModules();

	// Clean up all parser data structures, including global string buffer
	CleanupParser();

	if (yydebug)
	{
		// Print out some additional reference counts, for debugging
		printf("ExpressionArray::TotalRefCount after cleanup: %d\n", ExpressionArray::TotalRefCount);
	}

	// Return 0 if ok
	return ok ? 0 : 1;
}
