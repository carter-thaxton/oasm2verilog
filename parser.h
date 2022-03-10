
#ifndef PARSER_H
#define PARSER_H

#include "Common.h"
#include "Identifier.h"
#include "Function.h"
#include "SymbolTable.h"
#include "Expression.h"
#include "Signal.h"
#include "EnumValue.h"
#include "Module.h"
#include "Alu.h"
#include "AluInstruction.h"


enum ParseMode
{
	PARSE_UNKNOWN,
	PARSE_OASM,
	PARSE_EMBEDDED_OASM,
};

/*
 * Symbol table used during parsing
 */
extern SymbolTable *globalSymbols;
extern SymbolTable *symbols;

/*
 * Current module being declared
 */
extern Module *module;

/*
 * Current ALU instruction being declared
 */
extern AluInstruction *alu_instruction;

/*
 * List of parsed module definitions
 */
extern StringMap modules;

/*
 * Set to the current file type.
 * Normally set to PARSE_OASM, but is set to PARSE_VERILOG, for example,
 * when parsing Verilog code for embedded hot comments.
 * This variable is used by the lexer to setup its initial context.
 */
extern ParseMode parseMode;
extern ParseMode parseModeStart;


/*
 * Primary initialization of parser, to be called only once at program startup
 */
extern void InitParser();

/*
 * Primary call to lex and parse
 */
extern int ParseFile(FILE *file, const char *fname, ParseMode mode);

/*
 * Clean up parser, to be called only once at program shutdown after InitParser
 */
extern void CleanupParser();


/*
 * Calls used by parser
 */


// Create and finish parsing modules and objects
void startModule(const char *name);
void endModule();

void startExternModule(const char *name);
void endExternModule();

void startObject(const char *object, const char *name);
void endObject();




// Create new instances in current module
Instance *addInstance(const char *moduleName, const char *instanceName);

// Create new signals in current module
Signal *addSignal(const char *name, SignalBehavior behavior, SignalDataType dataType, SignalDirection dir, const Expression &expr);
void addSignals(const DottedIdentifierList &names, SignalBehavior behavior, SignalDataType dataType, SignalDirection dir, const Expression &expr);

// Create new connections in current module
void connect(const SignalReference &from, const SignalReference &to);

// Helper methods that call the above version of connect
void connect(const SignalReference &from, const SignalReferenceList &to);
void connect(const SignalReferenceList &from, const SignalReference &to);
void connect(const SignalReferenceList &from, const SignalReferenceList &to);


// Check that the given signal is locally declared in the current module
// This is a requirement of signals carried in expression values, and impacts the syntax of inner modules
bool checkLocalSignal(Signal *signal);

// Set initial values of registers, and indicate warm reset
void regInitialValues(const DottedIdentifierList &names, const Expression &expr);
void regWarmResetAffects(const DottedIdentifierList &names);

// Handle assignment statements
// Special-case assignment statements may appear in some sections.  This is a catch-all for the rest.
void assignment(const DottedIdentifierList &lhs, const Expression &rhs);


// Calls used during ALU instruction parsing
void startInstructions();
void endInstructions();

void tfaAssignment(const char *name, SignalBehavior behavior, const Expression &rhs);

AluInstruction *newInstruction(const char *name);
AluInstruction *ensureInstruction();
AluInstruction *endInstruction();
AluInstruction *instructionAssignment(DottedIdentifierList *lhs, const Expression &rhs);
AluInstruction *instructionSetV(const Expression &rhs);
AluInstruction *instructionGoto(const char *inst);
AluInstruction *instructionIf(const Expression &cond, const char *inst_if);
AluInstruction *instructionIfElse(const Expression &cond, const char *inst_if, const char *inst_else);
AluInstruction *instructionCase(const Expression &cond1, const Expression &cond0, const char *inst0, const char *inst1, const char *inst2, const char *inst3);
AluInstruction *instructionLatch(const DottedIdentifierList &rhs);
AluInstruction *instructionCondUpdateVR();
AluInstruction *instructionCondUpdateTF();
AluInstruction *instructionCondBypass();
AluInstruction *instructionWaitForV();


/*
 * Hooks to FLEX/BISON
 */

/* Hook to parse.y */
extern int yyparse(void);

/* Hooks to lex.l */
extern FILE *yyin;
extern int yylex(void);

/* Required hooks */
extern void yyerror(char const *errstr);
extern void yyerrorf(char const *errstr, ...);

/* Hook to parse.y */
#include "parse.tab.hpp"


#endif
