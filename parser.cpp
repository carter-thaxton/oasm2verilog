
#include "parser.h"
#include "SiliconObjectRegistry.h"
#include "BuiltinFunction.h"
#include "IllegalNames.h"
#include <typeinfo>

void InitParser()
{
	// Initialize string buffer for compilation unit
	strings = new StringBuffer();

	// Initialize built-in global symbol table, and start with global scope
	globalSymbols = new SymbolTable();
	symbols = globalSymbols;

	// Add built-in functions to symbol table
	InitializeBuiltinFunctions();

	// Initialize silicon objects
	// and add all enumerated values to global symbol table
	InitializeSiliconObjects();

	// Add reserved names to hashtable, to efficiently check for illegal signal and module names
	InitializeIllegalNames();
}

void CleanupParser()
{
	// Deletes silicon object definitions
	CleanupSiliconObjects();

	// Delete global symbol table
	// Note that this will also delete all symbols, including built-in functions and enumerated values
	if (symbols != globalSymbols)
		delete symbols;

	delete globalSymbols;  globalSymbols = NULL;  symbols = NULL;

	// Delete global string buffer
	delete strings;  strings = NULL;
}

int ParseFile(FILE *file, const char *fname, ParseMode mode)
{
	// Add filename to string buffer and set global variable that points to it
	if (fname)
		currentFilename = strings->AddString(fname);
	else
		currentFilename = NULL;

	// Open file
	yyin = file;

	// Setup lexer to expect the given file type
	parseModeStart = mode;
	parseMode = mode;

	// Start new file scope symbol table
	symbols = symbols->PushScope(new SymbolTable());

	// Parse input file
	int err = yyparse();

	// Pop back out of file scope
	symbols = symbols->PopScope();

	currentFilename = NULL;

	// yyparse returns 0 if no error, 1 if a parse error occurred, and 2 if a fatal error occurred.
	// Return the same value here.
	if (err)
		return err;

	// Return 1 if any semantic errors occurred
	if (errorCount > 0)
		return 1;

	// Return 0 if all ok
	return 0;
}


/*
 * Variables used by parser during construction and parsing
 */
SymbolTable *globalSymbols = NULL;
SymbolTable *symbols = NULL;
Module *module = NULL;
AluInstruction *alu_instruction = NULL;
StringMap modules;
ParseMode parseMode = PARSE_OASM;
ParseMode parseModeStart = PARSE_OASM;

// Keep track of current filename for error messages
const char *currentFilename = NULL;


/*
 * Calls used by parser during module construction
 */


// Create new instances in current module, referring to the module definition by name
Instance *addInstance(const char *moduleName, const char *instanceName)
{
	if (!module) return NULL;

	Instance *instance = new Instance(module, instanceName, moduleName);
	instance->Location = CurrentLocation();

	if (!symbols->Add(instance))
	{
		yyerrorf("Symbol already defined: '%s'", instanceName);
		delete instance;  instance = NULL;
		return NULL;
	}

	if (!module->AddInstance(instance))
	{
		yyfatalf("Could not add instance: '%s'", instanceName);
		return NULL;
	}


	return instance;
}


// Create new signal in current module
Signal *addSignal(const char *name, SignalBehavior behavior, SignalDataType dataType, SignalDirection dir, const Expression &expr)
{
	if (!module) return NULL;

	// Because anonymous signals are legal for constants, create a temporary name used for error messages
	bool anonymous = false;
	const char *tmp_name = name;
	if (name == NULL)
	{
		tmp_name = "(anonymous)";
		anonymous = true;
	}

	int value = -1;

	// Check initial value for validity
	if (behavior == BEHAVIOR_CONST || behavior == BEHAVIOR_REG)
	{
		if (expr.type == EXPRESSION_UNKNOWN)
		{
			// No initial value, use -1
			value = -1;
		}
		else if (expr.type == CONST_INT)
		{
			if (dataType == DATA_TYPE_BIT && (expr.val.i != 0 && expr.val.i != 1))
			{
				yyerrorf("Cannot initialize bit register or constant '%s' to values other than 0 or 1", tmp_name);
				return NULL;
			}

			// Always truncate constant to 16-bit word
			value = expr.val.i & 0xFFFF;
		}
		else
		{
			yyerrorf("Register values and constants must be integers: '%s'", tmp_name);
			return NULL;
		}
	}

	// If anonymous constant, first check for an existing constant
	if (name == NULL)
	{
		if (behavior != BEHAVIOR_CONST)
		{
			yyerror("Only constants may be anonymous");
			return NULL;
		}

		// Check for existing constants with the same value
		int n = module->SignalCount();
		for (int i=0; i < n; i++)
		{
			Signal *sig = module->GetSignal(i);

			// Return an existing constant with the same value, regardless of whether it is anonymous
			if ((sig->Behavior == BEHAVIOR_CONST) && (sig->InitialValue == value))
				return sig;
		}

		// Use the constant value as a name for the constant.
		// No other symbols are allowed to start with a digit or minus
		char anonymousName[64];
		sprintf(anonymousName, "%d", value);

		// Continue and create a new signal, but use the specially-formatted anonymous name
		name = strings->AddString(anonymousName);
	}
	else
	{
		// Check if name is prohibited
		if (IsIllegalSignalName(name))
		{
			yyerrorf("'%s' is an illegal signal name", name);
			return NULL;
		}
	}

	Signal *signal = new Signal(name, behavior, dataType, dir, value, anonymous, false);
	signal->Location = CurrentLocation();

	if (!module->AddSignal(signal))
	{
		delete signal;  signal = NULL;
		return NULL;
	}

	if (!symbols->Add(signal))
	{
		yyerrorf("Symbol already defined: '%s'", name);
		delete signal;  signal = NULL;
		return NULL;
	}

	return signal;
}

/* Add multiple signals */
void addSignals(const DottedIdentifierList &names, SignalBehavior behavior, SignalDataType dataType, SignalDirection dir, const Expression &expr)
{
	const DottedIdentifierList *s = &names;
	while (s)
	{
		if (s->Id.Depth() == 1)
		{
			addSignal(s->Id.Name, behavior, dataType, dir, expr);
		}
		else
		{
			yyerrorf("Dotted identifiers are not allowed in signal declarations: ");
			s->Id.Print(stderr);
		}
		s = s->Next;
	}
}


//
// Create new connections in current module
//

// Main method to perform connections
// These are resolved in the ResolveConnections pass
void connect(const SignalReference &from, const SignalReference &to)
{
	// Attempt to resolve connections to built-in signals at parse-time,
	// because these signals are not actually stored in module definitions.
	// Leave all other signals unresolved until ResolveConnections

	SignalReference from_temp = from;
	if (from.Id.Depth() == 1)
	{
		Symbol *s = symbols->Get(from.Id.Name);
		if (s && s->SymbolType() == SYMBOL_SIGNAL)
		{
			Signal *sig = (Signal *) s;
			if (sig->Behavior == BEHAVIOR_BUILTIN)
			{
				if (sig->Direction == DIR_IN || sig->Direction == DIR_OUT)
				{
					// allow connections from built-in inputs and outputs, but not internal signals
					from_temp.ResolvedSignal = sig;
					from_temp.ResolvedInstance = NULL;
				}
				else
				{
					yyerrorf("Cannot connect from built-in signal: %s", sig->Name());
					return;
				}
			}
		}
	}

	SignalReference to_temp = to;
	if (to.Id.Depth() == 1)
	{
		Symbol *s = symbols->Get(to.Id.Name);
		if (s && s->SymbolType() == SYMBOL_SIGNAL)
		{
			Signal *sig = (Signal *) s;
			if (sig->Behavior == BEHAVIOR_BUILTIN)
			{
				to_temp.ResolvedSignal = sig;
				if (sig->Direction == DIR_IN)
				{
					// allow connections to built-in inputs, but not outputs or internal signals
					to_temp.ResolvedSignal = sig;
					to_temp.ResolvedInstance = NULL;
				}
				else
				{
					yyerrorf("Cannot connect to built-in signal: %s", sig->Name());
					return;
				}
			}
		}
	}

	// Add connection to current module
	// If same connection was already made, does nothing and returns false
	Connection *c = new Connection(module, from_temp, to_temp, CurrentLocation());
	if (!module->AddConnection(c))
		delete c;
}

void connect(const SignalReference &from, const SignalReferenceList &to)
{
	// Used at the end of a signal connection to connect to a comma-separated list of destinations
	const DottedIdentifierList *ids = &to.Ids;

	// SignalReferenceList carries around a delay.  Use it from the source to the destinations.
	SignalReference src = from;
	src.DelayCount += to.DelayCount;

	while (ids)
	{
		connect(src, SignalReference::FromId(ids->Id, to.Direction));
		ids = ids->Next;
	}
}

void connect(const SignalReferenceList &from, const SignalReference &to, bool to_outer)
{
	// Connect from a list of identifiers.  This is only legal if there is only one identifer in the list.
	// This function exists because the parser accepts a right-arrow connection to the right of a comma-separated list of declarations,
	// but by the rule that only one source may drive a signal, using a list is illegal.
	if (from.Ids.Count() != 1)
	{
		yyerrorf("Cannot create a connection from multiple sources");
		return;
	}

	connect(SignalReference::FromId(from.Ids.Id, from.Direction), to);
}

void connect(const SignalReferenceList &from, const SignalReferenceList &to)
{
	// Connect from a list of identifiers.  This is only legal if there is only one identifer in the list.
	// This function exists because the parser accepts a right-arrow connection to the right of a comma-separated list of declarations,
	// but by the rule that only one source may drive a signal, using a list is illegal.
	if (from.Ids.Count() != 1)
	{
		yyerrorf("Cannot create a connection from multiple sources");
		return;
	}

	connect(SignalReference::FromId(from.Ids.Id, from.Direction), to);
}




// Check that the given signal is locally declared in the current module
// This is a requirement of signals carried in expression values, and impacts the syntax of inner modules
bool checkLocalSignal(Signal *signal)
{
	if (signal && signal->module)
	{
		if (signal->Behavior != BEHAVIOR_BUILTIN && signal->module != module)
		{
			yyerrorf("Signal '%s' is not declared locally within the module '%s'", signal->Name(), module->Name());
			return false;
		}
	}
	return true;
}

/* Set initial value for registers */
void regInitialValues(const DottedIdentifierList &names, const Expression &expr)
{
	if (expr.type != CONST_INT)
	{
		yyerrorf("Initial value for reg must be an integer");
		return;
	}

	// Truncate value to 16-bits
	int value = expr.val.i & 0xFFFF;

	for (const DottedIdentifierList *s = &names; s != NULL; s = s->Next)
	{
		if (s->Id.Depth() != 1)
		{
			yyerrorf("Dotted identifiers are not allowed in register declarations");
			s->Id.Print(stderr);
			continue;
		}

		Symbol *symbol = module->GetSignal(s->Id.Name);
		if (symbol)
		{
			if (symbol->SymbolType() == SYMBOL_SIGNAL)
			{
				Signal *sig = (Signal *) symbol;
				if (sig->Behavior == BEHAVIOR_REG)
				{
					if (sig->DataType == DATA_TYPE_BIT)
					{
						if (value != 0 && value != 1)
						{
							yyerrorf("Cannot initialize bit reg '%s' to values other than 0 or 1", sig->Name());
							continue;
						}
					}

					if (sig->InitialValue >= 0)
					{
						if (sig->InitialValue == value)
						{
							yywarnf("'%s' has already been initialized to %d", sig->Name(), sig->InitialValue);
						}
						else
						{
							yyerrorf("'%s' was already initialized to %d and is now being initialized to %d", sig->Name(), sig->InitialValue, value);
							continue;
						}
					}
					sig->InitialValue = value;
					continue;
				}
			}
		}
		yyerrorf("'%s' is not a register.  Cannot set initial value", s->Id.Name);
	}
}

/* Indicate registers affected by warm reset */
void regWarmResetAffects(const DottedIdentifierList &names)
{
	for (const DottedIdentifierList *s = &names; s != NULL; s = s->Next)
	{
		if (s->Id.Depth() != 1)
		{
			yyerrorf("Dotted identifiers are not allowed in warm_reset_affects declarations");
			s->Id.Print(stderr);
			continue;
		}

		Symbol *symbol = module->GetSignal(s->Id.Name);
		if (symbol)
		{
			if (symbol->SymbolType() == SYMBOL_SIGNAL)
			{
				Signal *sig = (Signal *) symbol;
				if (sig->Behavior == BEHAVIOR_REG)
				{
					if (sig->UsesWarmReset)
					{
						yywarnf("Register '%s' is already declared to be affected by warm_reset", sig->Name());
					}
					sig->UsesWarmReset = true;
					continue;
				}
			}
		}
		yyerrorf("'%s' is not a register.  Cannot set as warm_reset_affects", s->Id.Name);
	}
}



/*
 * Calls used to create and finish modules
 */

void startModule(const char *name)
{
	// Create a new concrete module definition (non-extern)
	module = new Module(name, module, false);
	module->Location = CurrentLocation();

	// Start a new symbol table and insert it in the scope chain
	symbols = symbols->PushScope(new SymbolTable());
}

void endModule()
{
	if (module == NULL)
	{
		// Report a fatal error if module is ever NULL at this point
		yyfatalf("module is NULL.  Problem with nested modules");
	}
	else if (IsIllegalModuleName(module->Name()))
	{
		yyerrorfl(module->Location, "'%s' is an illegal module name", module->Name());
	}
	else
	{
		// Perform analysis pass on each module as it is defined
		if (module->AnalyzeAfterParse())
		{
			// Add top-level modules to the global list of module definitions
			// Inner modules can be reached in later stages by walking the InnerModule list of the global modules
			if (module->ParentModule() == NULL)
			{
				if (modules.Add(module->Name(), module) == NULL)
				{
					Module *orig = (Module *) modules.Get(module->Name());
					if (orig->Location.Filename)
						yyerrorfl(module->Location, "Module '%s' already defined in %s on line %d", module->Name(), orig->Location.Filename, orig->Location.Line);
					else
						yyerrorfl(module->Location, "Module '%s' already defined on line %d", module->Name(), orig->Location.Line);
				}
			}
		}
	}

	// Pop out of current module scope
	if (module)
		module = module->ParentModule();

	// Pop out of the symbol table.  This will call appropriate destructors.
	symbols = symbols->PopScope();
}


void startExternModule(const char *name)
{
	// Create a new extern module definition
	module = new Module(name, module, true);
	module->Location = CurrentLocation();

	// Start a new symbol table and insert it in the scope chain
	symbols = symbols->PushScope(new SymbolTable());
}

void endExternModule()
{
	endModule();
}


void startObject(const char *object, const char *name)
{
	// Lookup object definition
	SiliconObjectDefinition *definition = LookupObjectDefinition(object);

	if (definition == NULL)
	{
		yyerrorf("'%s' does not name a silicon object type", object);

		// Use a simple module definition, to keep the parser happy from here forward,
		// and to maintain the scope chain when in an inner module
		module = new Module(name, module);
		module->Location = CurrentLocation();
	}
	
	else
	{
		// Create the new object from its definition
		// Note, this may use a registered factory method to return a subclass of SiliconObject
		module = definition->Create(name, module);
		module->Location = CurrentLocation();

		// When defined, insert a built-in symbol table between the existing scope and the new scope.
		SymbolTable *builtinSymbols = definition->Symbols();
		if (builtinSymbols)
			symbols = symbols->PushScope(builtinSymbols);
	}


	// Start a new symbol table and insert it in the scope chain.
	symbols = symbols->PushScope(new SymbolTable());
}


void endObject()
{
	endModule();
}



/*
 * Calls used during ALU instruction parsing
 */

void startInstructions()
{
	alu_instruction = NULL;
}

void endInstructions()
{
	endInstruction();
}

AluInstruction *newInstruction(const char *label)
{
	endInstruction();
	alu_instruction = new AluInstruction(label);
	alu_instruction->Location = CurrentLocation();
	return alu_instruction;
}

AluInstruction *ensureInstruction()
{
	// The first instruction in the 'inst' section need not supply a label.
	// This call is used from other functions adding to the current instruction to ensure that one exists.
	// If one does not exist, it has no label, and is anonymous, just as if it were preceded by ':'
	if (alu_instruction)
		return alu_instruction;
	else
		return newInstruction(NULL);
}

AluInstruction *endInstruction()
{
	// Complete the current instruction, if not already completed.
	// Apply defaults and add to ALU.
	// Return added instruction, which may be NULL if an error occurred.

	if (alu_instruction == NULL)
		return NULL;

	alu_instruction->ApplyDefaults();

	Alu *alu = (Alu *) module;
	AluInstruction *result = alu->AddInstruction(alu_instruction);

	// Clear current instruction
	alu_instruction = NULL;

	return result;
}

// Implements assignment statements inside of instructions.
// These can be AluFunctionCalls or TF overrides.
// Note that setting of the v-bit is parsed separately, and uses instructionSetV()
// If lhs is NULL, there was a bare rhs expression with no '=', which has meaning for AluFunctionCalls with no destination registers.
AluInstruction *instructionAssignment(DottedIdentifierList *lhs, const Expression &rhs)
{
	ensureInstruction();

	// If the RHS is an AluFunctionCall, set the instruction, and allow no destinations
	AluFunctionCall *instFcnCall = NULL;
	if (rhs.type == EXPRESSION_ALU_FCN)
	{
		instFcnCall = alu_instruction->SetFunction(rhs.val.fcn);
		if (instFcnCall == NULL)
		{
			yyerrorf("Cannot perform multiple instructions in one instruction slot");
			return NULL;
		}
	}

	// An expression with no LHS is illegal, unless it is an AluFunctionCall with no destinations
	else if (lhs == NULL)
	{
		yyerrorf("Unknown expression in ALU instruction");
		return NULL;
	}


	// Apply the RHS to the LHS, and ensure that data types match
	for (DottedIdentifierList *s = lhs; s != NULL; s = s->Next)
	{
		if (s->Id.Depth() != 1)
		{
			yyerrorf("Dotted identifiers are not allowed in instruction destinations: ");
			s->Id.Print(stderr);
			continue;
		}

		Symbol *symbol = symbols->Get(s->Id.Name);
		if (!symbol)
		{
			yyerrorf("Unknown destination register '%s' used in instruction", s->Id.Name);
			return NULL;
		}

		if (symbol->SymbolType() == SYMBOL_SIGNAL)
		{
			Signal *sig = (Signal *) symbol;
			if (sig->Behavior == BEHAVIOR_REG)
			{
				if (sig->DataType == DATA_TYPE_WORD)
				{
					// Allow CONST_INT values on RHS of an assignment to a word reg in an instruction.
					// This infers a mov() or zero() instruction
					if (instFcnCall == NULL)
					{
						if (rhs.type == CONST_INT)
						{
							Expression inferredAluFcn;

							if (rhs.val.i == 0)
								inferredAluFcn = Function::Call(symbols, "zero", 0, NULL, NULL, NULL, NULL);
							else
								inferredAluFcn = Function::Call(symbols, "mov",  1, &rhs, NULL, NULL, NULL);

							instFcnCall = alu_instruction->SetFunction(inferredAluFcn.val.fcn);
							if (instFcnCall == NULL)
							{
								yyerrorf("Inferring a mov() ALU instruction, but cannot perform multiple instructions in one instruction slot.");
								return NULL;
							}
						}
						else
						{
							yyerrorf("Only instructions and constants may be assigned to word regs in an ALU instruction");
							return NULL;
						}
					}

					// Add destination register
					if (alu_instruction->AddDestination(sig) == NULL)
					{
						yyerrorf("Illegal destination register '%s' in instruction", sig->Name());
						return NULL;
					}
					continue;
				}
				else if (sig->DataType == DATA_TYPE_BIT)
				{
					int bit_val = rhs.val.i;
					if (bit_val != 0 && bit_val != 1)
					{
						yyerrorf("TF override assignments may only be to values 0 and 1");
						return NULL;
					}

					if (alu_instruction->AddTFOverride(sig, bit_val) == NULL)
					{
						yyerrorf("Illegal TF override of '%s' in instruction", sig->Name());
						return NULL;
					}
					continue;
				}
			}
		}
		
		yyerrorf("Destination register '%s' is not declared as a reg", s->Id.Name);
		return NULL;
	}

	return alu_instruction;
}

// Sets the 'v' output bit.
AluInstruction *instructionSetV(const Expression &rhs)
{
	ensureInstruction();

	int bit_val = -1;

	const Signal *v_in_signal = (Signal *) Alu::BuiltinDefinition()->Symbols()->Get("v_in");

	if (rhs.type == EXPRESSION_SIGNAL && rhs.val.sig == v_in_signal)
	{
		bit_val = 2;
	}
	else if (rhs.type == CONST_ENUM && strcmp(rhs.val.s, "hold") == 0)
	{
		bit_val = 3;
	}
	else if (rhs.type == CONST_INT)
	{
		bit_val = rhs.val.i;
		if (bit_val != 0 && bit_val != 1)
		{
			bit_val = -1;
		}
	}

	if (bit_val < 0)
	{
		yyerrorf("Illegal setting for v-bit in instruction.  Must be 0, 1, v_in, or hold");
		return NULL;
	}

	if (!alu_instruction->SetVOut(bit_val))		// Note, may be 0, 1, 2 (v_in), or 3 (hold)
	{
		yyerrorf("v-bit has already been set in this instruction");
		return NULL;
	}

	return alu_instruction;
}


// Helper function used to resolve branch conditions to a branch signal.
// It is possible to use expression logic in the branch conditions, as long it resolves
// to a simple function of zero or one branch register.
static bool resolveBranchCondition(const Expression &cond, Signal *&signal, bool &inverse, bool &const_result)
{
	signal = NULL;
	inverse = false;
	const_result = false;

	bool ok = false;

	if (cond.type == EXPRESSION_TF)
	{
		if (cond.val.tf.Logic == 0xFFFF)
		{
			const_result = true;
			inverse = false;
			ok = true;
		}
		else if (cond.val.tf.Logic == 0x0000)
		{
			const_result = true;
			inverse = true;
			ok = true;
		}
		else if (cond.val.tf.NumArgs() == 1)
		{
			if (cond.val.tf.Logic == 0xAAAA)
			{
				signal = cond.val.tf.Args[0];
				inverse = false;
				ok = true;
			}
			else if (cond.val.tf.Logic == 0x5555)
			{
				signal = cond.val.tf.Args[0];
				inverse = true;
				ok = true;
			}
		}
	}
	else if (cond.type == EXPRESSION_SIGNAL)
	{
		signal = cond.val.sig;
		ok = true;
	}
	else if (cond.type == CONST_INT)
	{
		if (cond.val.i == 0)
		{
		}
		else if (cond.val.i == 1)
		{
		}
	}

	if (signal && signal->Behavior != BEHAVIOR_BRANCH)
	{
		ok = false;
	}

	if (!ok)
	{
		yyerrorf("Branch condition must be a simple expression of one TFA branch register");
	}

	return ok;
}

AluInstruction *instructionGoto(const char *inst)
{
	ensureInstruction();

	if (!alu_instruction->Goto(inst))
	{
		yyerrorf("Branch destination already set for instruction");
		return NULL;
	}

	return alu_instruction;
}

AluInstruction *instructionIf(const Expression &cond, const char *inst_if)
{
	return instructionIfElse(cond, inst_if, NULL);
}

AluInstruction *instructionIfElse(const Expression &cond, const char *inst_if, const char *inst_else)
{
	ensureInstruction();

	bool inverse = false;
	bool const_result = false;
	Signal *cond_signal;

	if (!resolveBranchCondition(cond, cond_signal, inverse, const_result))
		return NULL;

	bool ok = false;
	if (const_result)
	{
		if (!inverse)
		{
			return instructionGoto(inst_if);
		}
		else
		{
			// if (0) -- ignore
		}
	}
	else
	{
		if (inverse)
		{
			ok = alu_instruction->IfElse(cond_signal, NULL, inst_if);
		}
		else
		{
			ok = alu_instruction->IfElse(cond_signal, inst_if, NULL);
		}
	}

	if (!ok)
	{
		yyerrorf("Branch destination already set for instruction");
		return NULL;
	}

	return alu_instruction;
}

AluInstruction *instructionCase(const Expression &cond1, const Expression &cond0, const char *inst0, const char *inst1, const char *inst2, const char *inst3)
{
	ensureInstruction();

	bool inverse1;
	bool const_result1;
	Signal *cond_signal1;

	if (!resolveBranchCondition(cond1, cond_signal1, inverse1, const_result1))
		return NULL;

	if (const_result1)
	{
		// Result of just cond0
		if (inverse1)
		{
			return instructionIfElse(cond0, inst0, inst1);
		}
		else
		{
			return instructionIfElse(cond0, inst2, inst3);
		}
	}

	bool inverse0;
	bool const_result0;
	Signal *cond_signal0;

	if (!resolveBranchCondition(cond0, cond_signal0, inverse0, const_result0))
		return NULL;

	if (const_result0)
	{
		// Result of just cond1
		if (inverse1)
		{
			return instructionIfElse(cond0, inst0, inst2);
		}
		else
		{
			return instructionIfElse(cond0, inst1, inst3);
		}
	}

	// Because of inverse expressions in conditions, the case may apply up to 4 different orders
	bool ok = false;
	if (!inverse1 && !inverse0)
	{
		ok = alu_instruction->Case(cond_signal1, cond_signal0,  inst0, inst1, inst2, inst3);
	}
	else if (!inverse1 && inverse0)
	{
		ok = alu_instruction->Case(cond_signal1, cond_signal0,  inst1, inst0, inst3, inst2);
	}
	else if (inverse1 && !inverse0)
	{
		ok = alu_instruction->Case(cond_signal1, cond_signal0,  inst2, inst3, inst0, inst1);
	}
	else if (inverse1 && inverse0)
	{
		ok = alu_instruction->Case(cond_signal1, cond_signal0,  inst3, inst2, inst1, inst0);
	}

	if (!ok)
	{
		yyerrorf("Branch destination already set for instruction");
		return NULL;
	}

	return NULL;
}

AluInstruction *instructionLatch(const DottedIdentifierList &rhs)
{
	ensureInstruction();

	// Lookup each signal, and ensure that it is a valid word_in
	for (const DottedIdentifierList *s = &rhs; s != NULL; s = s->Next)
	{
		if (s->Id.Depth() != 1)
		{
			yyerrorf("Dotted identifiers are not allowed in latch signals: ");
			s->Id.Print(stderr);
			continue;
		}

		Symbol *symbol = symbols->Get(s->Id.Name);
		if (!symbol)
		{
			yyerrorf("Unknown signal '%s' used in latch", s->Id.Name);
			return NULL;
		}

		if (symbol->SymbolType() != SYMBOL_SIGNAL)
		{
			yyerrorf("Illegal signal used in latch '%s'", s->Id.Name);
			return NULL;
		}

		Signal *sig = (Signal *) symbol;
		if ((sig->Behavior != BEHAVIOR_WIRE && sig->Behavior != BEHAVIOR_DELAY) || sig->DataType != DATA_TYPE_WORD)
		{
			yyerrorf("Illegal signal used in latch '%s'.  Only non-register word signals may be latched", sig->Name()); 
			return NULL;
		}

		// Add latch of word_in
		if (alu_instruction->AddLatch(sig) == NULL)
		{
			yyerrorf("Illegal signal used in latch '%s'", sig->Name());
			return NULL;
		}
	}

	return alu_instruction;
}

AluInstruction *instructionCondUpdateVR()
{
	ensureInstruction();
	if (!alu_instruction->SetCondUpdateVR())
	{
		yyerrorf("cond_update_vr() may only be set once per instruction");
		return NULL;
	}
	return alu_instruction;
}

AluInstruction *instructionCondUpdateTF()
{
	ensureInstruction();
	if (!alu_instruction->SetCondUpdateTF())
	{
		yyerrorf("cond_update_tf() may only be set once per instruction");
		return NULL;
	}
	return alu_instruction;
}

AluInstruction *instructionCondBypass()
{
	ensureInstruction();
	if (!alu_instruction->SetCondBypass())
	{
		yyerrorf("cond_bypass() may only be set once per instruction");
		return NULL;
	}
	return alu_instruction;
}

AluInstruction *instructionWaitForV()
{
	ensureInstruction();
	if (!alu_instruction->SetWaitForV())
	{
		yyerrorf("wait_for_v() may only be set once per instruction");
		return NULL;
	}
	return alu_instruction;
}

/*
 * Handle assignment statement at module and file scope
 * There are special-case assignment statements in init, inst, and tfa sections
 */
void assignment(const DottedIdentifierList &lhs, const Expression &rhs)
{
	for (const DottedIdentifierList *s = &lhs; s != NULL; s = s->Next)
	{
		if (s->Id.Depth() != 1)
		{
			//FIXME add support for assignment to dotted parameters
			yyerrorf("Dotted identifiers are not allowed in assignment statements: ");
			s->Id.Print(stderr);
			//FIXME memory leak -- s->Id should be deleted by whatever handles this
			continue;
		}

		Symbol *symbol = symbols->Get(s->Id.Name);
		if (symbol == NULL)
		{
			yyerrorf("Symbol '%s' not found in assignment", s->Id.Name);
		}
		else
		{
			// Compile-time variable assignment
			if (symbol->SymbolType() == SYMBOL_VARIABLE)
			{
				Variable *var = (Variable *) symbol;

				// Delete old value
				var->Value.Delete();

				// Replace with new value
				var->Value = rhs;
				rhs.IncRef();
			}

			// Assignment to a parameter
			else if (symbol->SymbolType() == SYMBOL_PARAMETER)
			{
				ParameterDefinition *def = (ParameterDefinition *) symbol;

				Parameter *param = new Parameter(def, rhs);
				param->Location = CurrentLocation();

				if (!module->AddParameter(param))
				{
					delete param;  param = NULL;
				}
			}

			// Check if assigning to a signal
			else if (symbol->SymbolType() == SYMBOL_SIGNAL)
			{
				// Assignment to a signal is only valid in a TF_Module (ALU or FloatingTF)
				TF_Module *tf_module = dynamic_cast<TF_Module*>(module);
				if (tf_module == NULL)
				{
					yyerrorf("Illegal assignment to signal '%s'", s->Id.Name);
				}
				else
				{
					Signal *sig = (Signal *) symbol;
					if (sig->Behavior == BEHAVIOR_REG && sig->DataType == DATA_TYPE_BIT)
					{
						TruthFunction tf = rhs.ToTF();
						tf_module->AddTF(sig, tf);
						continue;
					}
					yyerrorf("Signal '%s' not declared as a bit reg in TF assignment", s->Id.Name);
				}
			}
		}
	}
}


// Special-case assignment statement in 'tfa' section
void tfaAssignment(const char *name, SignalBehavior behavior, const Expression &rhs)
{
	Alu *alu = (Alu *) module;
	TruthFunction tf = rhs.ToTF();

	Signal *sig = addSignal(name, behavior, DATA_TYPE_BIT, DIR_NONE, Expression::Unknown());
	if (!sig) return;

	alu->AddTFA(sig, tf);
}
