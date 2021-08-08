
%{
#include "parser.h"
%}

/* Parser options */
%defines
%locations

/* Semantic value types */
%union {
	char *str;
	DottedIdentifier id;
	DottedIdentifierList idlist;
	SignalReference sigref;
	SignalReferenceList sigreflist;
	Expression expr;
}

/* Keywords */
%token			_INCLUDE_ _NAMESPACE_ _MODULE_ _EXTERN_
%token			_ALU_ _TF_ _FPOA_
%token			_BIT_ _WORD_ _INPUT_ _OUTPUT_ _REG_ _WIRE_ _CONST_ _VAR_ _DELAY_ _V_ _DOT_V_
%token			_IF_ _ELSE_ _GOTO_ _CASE_ _LATCH_
%token			_LE_ _GE_ _EQ_ _NE_ _AND_ _OR_ _LSHIFT_ _RSHIFT_ _LARROW_ _RARROW_
%token			_INIT_ _TFA_ _INST_
%token			_BRANCH_ _COND_UPDATE_ _COND_BYPASS_ _COND_UPDATE_VR_ _COND_UPDATE_TF_ _WAIT_FOR_V_ _WARM_RESET_AFFECTS_

%token			_PRINT_	/* DEBUG */

/* Tokens with semantic value */
%token <str>		_ID_
%token <expr>		_VALUE_

/* Operators in increasing order of precedence */
%left  <expr>		_LARROW_ _RARROW_
%right <expr>		'?' ':'
%left  <expr>		_OR_
%left  <expr>		_AND_
%left  <expr>		'|'
%left  <expr>		'^'
%left  <expr>		'&'
%left  <expr>		_EQ_ _NE_
%left  <expr>		'<' '>' _LE_ _GE_
%left  <expr>		_LSHIFT_ _RSHIFT_
%left  <expr>		'+' '-'
%left  <expr>		'*' '/' '%'
%right <expr>		_UNARY_MINUS_ '!' '~'

%right			'.'				/* dotted identifiers */
%nonassoc		_DOT_V_				/* v-bit binds tighter than other dot references. */
%right			'[' ']'				/* bit-slices */

/* Non-terminal types */
%type  <str>		alu_inst_label
%type  <id>		dotted_id
%type  <idlist>		id_list dotted_id_list
%type  <sigref>		left_signal_expr signal_src
%type  <sigreflist>	right_signal_expr input_decl_base output_decl_base wire_decl_base reg_decl_base
%type  <expr>		expr array_expr fcn_call

%%



/*
 *	File Scope
 */

file_stmts		:	/* empty */
			| file_stmts file_stmt

file_stmt		: common_stmt
			| include_stmt
			| extern_module_decl
			| module_decl
			| object_decl
			| assign_stmt


/*
 *	Common Productions
 */

common_stmt		: ';'	/* empty statement */
			| var_decl
			| print_stmt


var_decl		: _VAR_ _ID_ '=' expr ';'	{	/* Variable definition */
								if (!symbols->AddVariable($2, $4))
									yyerrorf("Variable '%s' already defined", $2);
								$4.Delete();
							}

			| _VAR_ _ID_ ';'		{	/* Variable declaration with undefined initial value */
								if (!symbols->AddVariable($2, Expression::Unknown()))
									yyerrorf("Variable '%s' already defined", $2);
							}

			| _VAR_ _ID_ '[' ']' ';'	{	/* Variable declaration for zero-sized array */
								Expression array = Expression::Array();
								if (!symbols->AddVariable($2, array))
									yyerrorf("Variable '%s' already defined", $2);
								array.Delete();
							}

			| _VAR_ _ID_ '[' expr ']' ';'	{	/* Variable declaration for sized array with undefined initial values */
								int sz = $4.ToInt();

								if (sz < 0)
								{
									yyerrorf("Illegal initial size for array: %d", sz);
									sz = 0;
								}

								/* Even though errors may have been reported, add the variable, to prevent further errors */
								Expression array = Expression::Array(sz);
								if (!symbols->AddVariable($2, array))
									yyerrorf("Variable '%s' already defined", $2);

								array.Delete();
								$4.Delete();
							}

			| _VAR_ _ID_ '[' ']' '=' expr ';'
							{	/* Variable declaration for implicitly sized array with initial values */
								if ($6.type != EXPRESSION_ARRAY)
								{
									yyerrorf("Cannot initialize an array with a non-array value");
								}

								/* Even though errors may have been reported, add the variable, to prevent further errors */
								if (!symbols->AddVariable($2, $6))
									yyerrorf("Variable '%s' already defined", $2);

								$6.Delete();
							}

			| _VAR_ _ID_ '[' expr ']' '=' expr ';'
							{	/* Variable declaration for explicitly sized array with initial values */
								int sz = $4.ToInt();

								if ($7.type != EXPRESSION_ARRAY)
								{
									yyerrorf("Cannot initialize an array with a non-array value");
								}
								else if (sz < 0)
								{
									yyerrorf("Illegal initial size for array: %d", sz);
								}
								else if ($7.Length() != sz)
								{
									yyerrorf("Array is declared to contain %d values, but is initialized with %d values", sz, $7.Length());
								}

								/* Even though errors may have been reported, add the variable, to prevent further errors */
								if (!symbols->AddVariable($2, $7))
									yyerrorf("Variable '%s' already defined", $2);

								$4.Delete();
								$7.Delete();
							}


print_stmt		: _PRINT_ expr ';'		{	/* Print value of expression - useful for debug */
								$2.Print(stdout);
								$2.Delete();
							}


include_stmt		: _INCLUDE_ expr ';'		{	/* Include statement */
								if ($2.type == CONST_STRING)
								{
									/* Not yet supported */
									yyerror("'include' statement not yet supported");
								}
								else
									{
									yyerror("Invalid 'include' statement");
								}
								$2.Delete();
							}

/*
 * Identifier types:
	_ID_		- Single string
	dotted_id	- List of dotted strings
	dotted_id_list	- List of dotted_ids
	id_list		- List of dotted_ids, but constrained to contain only single-string identifiers
 */

dotted_id		: _ID_				{	/* Single-string DottedIdentifier */
								$$.Name = $1;
								$$.Next = NULL;
							}

			| _ID_ '.' dotted_id		{	/* Create new list item of DottedIdentifier */
								/* Note: This list is parsed 'backwards', so that the created linked list is 'forwards' */
								DottedIdentifier *id = new DottedIdentifier();
								id->Name = $3.Name;
								id->Next = $3.Next;

								$$.Name = $1;
								$$.Next = id;
							}


dotted_id_list		: dotted_id			{	/* Single-element DottedIdentifier list */
								$$.Id = $1;
								$$.Next = NULL;
							}
			
			| dotted_id ',' dotted_id_list	{	/* Create new list item of DottedIdentifier.  These should be deleted when used. */
								DottedIdentifierList *idlist = new DottedIdentifierList();
								idlist->Id = $3.Id;
								idlist->Next = $3.Next;

								$$.Id = $1;
								$$.Next = idlist;
							}


id_list			: _ID_				{	/* Single-string list */
								$$.Id.Name = $1;
								$$.Id.Next = NULL;
								$$.Next = NULL;
							}
			| _ID_ ',' id_list		{	/* Create new list item of strings.  These should be deleted when used. */
								/* Note: This list is parsed 'backwards', so that the created linked list is 'forwards' */
								DottedIdentifierList *idlist = new DottedIdentifierList();
								idlist->Id = $3.Id;
								idlist->Next = $3.Next;

								$$.Id.Name = $1;
								$$.Id.Next = NULL;
								$$.Next = idlist;
							}

optional_parens		: /* empty */
			| '(' ')'


/*
 *      Module/Object Declaration
 */


/* User-defined module */
module_decl		: _MODULE_ _ID_				{ startModule($2); }
				'{' module_stmts '}'		{ endModule(); }


module_stmts		:	/* empty */
			| module_stmts module_stmt


module_stmt		: common_module_stmt
			| instance_stmt
			| module_decl
			| object_decl


/* Instantiation of user-defined module */
instance_stmt		: _ID_ _ID_ ';'				{ addInstance($1, $2); }


/* Statements common to all modules */
common_module_stmts	:	/* empty */
			| common_module_stmts common_module_stmt


common_module_stmt	: common_stmt
			| port_decl
			| wire_decl
			| connect_decl
			| assign_stmt


port_decl		: input_decl
			| output_decl


input_decl		: input_decl_base ';'			{	/* No connection in declaration */
									$1.Delete();
								}

			| input_decl_base _RARROW_ right_signal_expr ';'
								{	/* Inner connection through a signal expression to the right */
									connect($1, $3);
									$1.Delete();
									$3.Delete();
								}

			| input_decl_base _LARROW_ left_signal_expr ';'
								{	/* Outer connection from a signal expression to the left */
									connect($3, $1);
									$1.Delete();
								}



output_decl		: output_decl_base ';'			{	/* No connection in declaration */
									$1.Delete();
								}

			| output_decl_base _RARROW_ right_signal_expr ';'
								{	/* Outer connection through a signal expression to the right */
									connect($1, $3);
									$1.Delete();
									$3.Delete();
								}

			| output_decl_base _LARROW_ left_signal_expr ';'
								{	/* Inner connection from a signal expression to the left */
									connect($3, $1);
									$1.Delete();
								}



input_decl_base		: _INPUT_ _WORD_  id_list		{ addSignals($3, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_IN); }
			| _INPUT_ _WIRE_ _WORD_ id_list		{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_IN); }
			| _INPUT_ _WORD_ _WIRE_ id_list		{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_IN); }
			| _INPUT_ _BIT_   id_list		{ addSignals($3, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_IN); }
			| _INPUT_ _BIT_  _WIRE_ id_list		{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_IN); }
			| _INPUT_ _WIRE_ _BIT_  id_list		{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_IN,     Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_IN); }


output_decl_base	: _OUTPUT_ _WORD_  id_list		{ addSignals($3, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_OUT); }
			| _OUTPUT_ _WIRE_ _WORD_ id_list	{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _OUTPUT_ _WORD_ _WIRE_ id_list	{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_WORD, DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _OUTPUT_ _BIT_   id_list		{ addSignals($3, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_OUT); }
			| _OUTPUT_ _WIRE_ _BIT_  id_list	{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _OUTPUT_ _BIT_  _WIRE_ id_list	{ addSignals($4, BEHAVIOR_WIRE, DATA_TYPE_BIT,  DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }


wire_decl		: wire_decl_base ';'			{	/* Simple wire declaration */
									$1.Delete();
								}

			| wire_decl_base _LARROW_ left_signal_expr ';'
								{	/* Wire declaration with source signal expression */
									connect($3, $1);
									$1.Delete();
								}

			| wire_decl_base _RARROW_ right_signal_expr ';'
								{	/* Wire declaration through signal expression to the right */
									connect($1, $3);
									$1.Delete();
									$3.Delete();
								}


wire_decl_base		: _WORD_  id_list			{ addSignals($2, BEHAVIOR_WIRE,  DATA_TYPE_WORD, DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($2, DIR_NONE); }
			| _WORD_ _WIRE_ id_list			{ addSignals($3, BEHAVIOR_WIRE,  DATA_TYPE_WORD, DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }
			| _WIRE_ _WORD_ id_list			{ addSignals($3, BEHAVIOR_WIRE,  DATA_TYPE_WORD, DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }

			| _BIT_   id_list			{ addSignals($2, BEHAVIOR_WIRE,  DATA_TYPE_BIT,  DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($2, DIR_NONE); }
			| _BIT_  _WIRE_ id_list			{ addSignals($3, BEHAVIOR_WIRE,  DATA_TYPE_BIT,  DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }
			| _WIRE_ _BIT_  id_list			{ addSignals($3, BEHAVIOR_WIRE,  DATA_TYPE_BIT,  DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }



reg_decl		: reg_decl_base ';'			{	/* Simple register declaration */
									$1.Delete();
								}

			| reg_decl_base '=' expr ';'		{	/* Register with initial value */
									regInitialValues($1.Ids, $3);
									$1.Delete();
									$3.Delete();
								}

			| reg_decl_base _RARROW_ right_signal_expr ';'
								{	/* Register with connection through signal expression to the right */
									connect($1, $3);
									$1.Delete();
									$3.Delete();
								}

			| reg_decl_base '=' expr _RARROW_ right_signal_expr ';'
								{	/* Register with initial value and connection through signal expression to the right */
									regInitialValues($1.Ids, $3);
									connect($1, $5);
									$1.Delete();
									$3.Delete();
									$5.Delete();
								}


reg_decl_base		: _OUTPUT_  _WORD_ _REG_  id_list	{ addSignals($4, BEHAVIOR_REG,   DATA_TYPE_WORD, DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _OUTPUT_  _REG_  _WORD_ id_list	{ addSignals($4, BEHAVIOR_REG,   DATA_TYPE_WORD, DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _WORD_ _REG_   id_list		{ addSignals($3, BEHAVIOR_REG,   DATA_TYPE_WORD, DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }
			| _REG_  _WORD_  id_list		{ addSignals($3, BEHAVIOR_REG,   DATA_TYPE_WORD, DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }

			| _OUTPUT_ _BIT_ _REG_ id_list		{ addSignals($4, BEHAVIOR_REG,   DATA_TYPE_BIT,  DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _OUTPUT_ _REG_ _BIT_ id_list		{ addSignals($4, BEHAVIOR_REG,   DATA_TYPE_BIT,  DIR_OUT,    Expression::Unknown());  $$ = SignalReferenceList::FromIdList($4, DIR_OUT); }
			| _BIT_ _REG_  id_list			{ addSignals($3, BEHAVIOR_REG,   DATA_TYPE_BIT,  DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }
			| _REG_ _BIT_  id_list			{ addSignals($3, BEHAVIOR_REG,   DATA_TYPE_BIT,  DIR_NONE,   Expression::Unknown());  $$ = SignalReferenceList::FromIdList($3, DIR_NONE); }



connect_decl		: dotted_id_list _LARROW_ left_signal_expr ';'
								{	/* Connect through a signal expression to the left */
									connect($3, SignalReferenceList::FromIdList($1, DIR_NONE));
									$1.Delete();
								}

			| signal_src _RARROW_ right_signal_expr ';'
								{	/* Connect through a signal expression to the right */
									connect($1, $3);
									$3.Delete();
								}



const_decl		: _CONST_ _WORD_  id_list '=' expr ';'	{ addSignals($3, BEHAVIOR_CONST, DATA_TYPE_WORD,  DIR_NONE,  $5);  $3.Delete();  $5.Delete(); }
			| _CONST_ _BIT_   id_list '=' expr ';'	{ addSignals($3, BEHAVIOR_CONST, DATA_TYPE_BIT,   DIR_NONE,  $5);  $3.Delete();  $5.Delete(); }
			| _WORD_  _CONST_ id_list '=' expr ';'	{ addSignals($3, BEHAVIOR_CONST, DATA_TYPE_WORD,  DIR_NONE,  $5);  $3.Delete();  $5.Delete(); }
			| _BIT_   _CONST_ id_list '=' expr ';'	{ addSignals($3, BEHAVIOR_CONST, DATA_TYPE_BIT,   DIR_NONE,  $5);  $3.Delete();  $5.Delete(); }



warm_reset_stmt		: _WARM_RESET_AFFECTS_ '(' id_list ')' ';'
								{	/* Indicate which registers need warm reset */
									regWarmResetAffects($3);
									$3.Delete();
								}

assign_stmt		: dotted_id_list '=' expr ';'		{	/* Assignment statement to a list of destinations */
									/* Note: Dotted identifiers aren't handled yet, but we plan on adding support
									   for dotted parameter assignments. */
									assignment($1,$3);
									$1.Delete();
									$3.Delete();
								}

			| _ID_ '[' expr ']' '=' expr ';'	{	/* Assignment statement to an array variable */
									Symbol *s = symbols->Get($1);
									if (!s)
									{
										yyerrorf("Symbol '%s' not defined", $1);
									}
									else if (s->SymbolType() == SYMBOL_VARIABLE)
									{
										Variable *var = (Variable *) s;
										var->Value.SetArrayValue($3.ToInt(), $6);
									}
									else
									{
										yyerrorf("Array assignment may only be performed on variables: '%s'", $1);
									}
									$3.Delete();
									$6.Delete();
								}


/* Extern definition of module */
extern_module_decl	: _EXTERN_ _MODULE_ _ID_		{ startExternModule($3); }
				'{' extern_module_stmts '}'	{ endExternModule(); }


extern_module_stmts	:	/* empty */
			| extern_module_stmts extern_module_stmt


/* Only ports are allowed in extern modules */
extern_module_stmt	: common_stmt
			| extern_port_decl


extern_port_decl	: input_decl_base ';'			{ $1.Delete(); }
			| output_decl_base ';'			{ $1.Delete(); }






/* Silicon objects, both special and simple */
object_decl		: fpoa_decl
			| alu_decl
			| tf_decl
			| simple_object_decl


/* Top-level FPOA */

/* Parses just like a module, but uses an FPOA object definition,
   to give it a set of parameters for the FPOA_CONTROL object */
fpoa_decl		: _FPOA_ _ID_				{ startObject("FPOA", $2); }
				'{' module_stmts '}'		{ endObject(); }


/* ALU */
alu_decl		: _ALU_ _ID_				{ startObject("ALU", $2); }
				'{' alu_stmts '}'		{ endObject(); }


alu_stmts		:	/* empty */
			| alu_stmts alu_stmt


alu_stmt		: common_module_stmt
			| reg_decl
			| const_decl
			| alu_init_decl
			| alu_tfa_decl
			| alu_inst_decl
			| alu_tf_decl
			| warm_reset_stmt


alu_init_decl		: _INIT_
				'{' alu_init_stmts '}'


alu_init_stmts		:	/* empty */
			| alu_init_stmts alu_init_stmt


alu_init_stmt		: common_stmt
			| warm_reset_stmt
			| id_list '=' expr ';'			{	/* Initial values for registers */
									regInitialValues($1,$3);
									$1.Delete();
									$3.Delete();
								}

alu_tfa_decl		: _TFA_
				'{' alu_tfa_stmts '}'


alu_tfa_stmts		:	/* empty */
			| alu_tfa_stmts alu_tfa_stmt


alu_tfa_stmt		: common_stmt

			| _BRANCH_ _ID_ '=' expr ';'		{	/* TFA branch */
									tfaAssignment($2, BEHAVIOR_BRANCH, $4);
									$4.Delete();
								}

			| _COND_BYPASS_ '=' expr ';'		{	/* Anonymous conditional bypass */
									tfaAssignment("cond_bypass", BEHAVIOR_COND_BYPASS, $3);
									$3.Delete();
								}
			| _COND_BYPASS_ _ID_ '=' expr ';'	{	/* Named conditional bypass */
									tfaAssignment($2, BEHAVIOR_COND_BYPASS, $4);
									$4.Delete();
								}

			| _COND_UPDATE_ '=' expr ';'		{	/* Anonymous conditional update */
									tfaAssignment("cond_update", BEHAVIOR_COND_UPDATE, $3);
									$3.Delete();
								}
			| _COND_UPDATE_ _ID_ '=' expr ';'	{	/* Named conditional update */
									tfaAssignment($2, BEHAVIOR_COND_UPDATE, $4);
									$4.Delete();
								}


alu_inst_decl		: _INST_				{ startInstructions(); }
				'{' alu_inst_stmts '}'		{ endInstructions(); }


alu_inst_stmts		: /* empty */
			| alu_inst_stmts alu_inst_stmt


alu_inst_stmt		: common_stmt

			| alu_inst_label			{	/* Start new instruction */
									newInstruction($1);
								}

			| id_list '=' expr ';'			{	/* Assignment within ALU instruction (instruction function, TF override, v-bit) */
									instructionAssignment(&$1, $3);
									$1.Delete();
									$3.Delete();
								}

			| fcn_call ';'				{	/* Instruction with no destination */
									instructionAssignment(NULL, $1);
									$1.Delete();
								}
			
			| _V_ '=' expr ';'			{	/* Set V-bit destination of ALU instruction */
									instructionSetV($3);
									$3.Delete();
								}

			| _GOTO_ _ID_ ';'			{	/* Branch Goto */
									instructionGoto($2);
								}

			| _IF_ '(' expr ')' _ID_ ';'		{	/* Branch If */
									instructionIf($3, $5);
									$3.Delete();
								}

			| _IF_ '(' expr ')' _ID_ _ELSE_ _ID_ ';'
								{	/* Branch If-Else */
									instructionIfElse($3, $5, $7);
									$3.Delete();
								}

			| _CASE_ '(' expr ',' expr ')' _ID_ ',' _ID_ ',' _ID_ ',' _ID_ ';'
								{	/* Branch Case */
									instructionCase($3, $5,   $7, $9, $11, $13);
									$3.Delete();
									$5.Delete();
								}

			| _LATCH_ '(' id_list ')' ';'		{	/* ALU Input Latch */
									instructionLatch($3);
									$3.Delete();
								}

			| _COND_UPDATE_VR_ optional_parens ';'	{	/* Conditional Update VR Destination */
									instructionCondUpdateVR();
								}

			| _COND_UPDATE_TF_ optional_parens ';'	{	/* Conditional Update TF */
									instructionCondUpdateTF();
								}

			| _COND_BYPASS_ optional_parens ';'	{	/* Conditional Bypass Destination */
									instructionCondBypass();
								}

			| _WAIT_FOR_V_ optional_parens ';'	{	/* Wait For V-bit */
									instructionWaitForV();
								}


alu_inst_label		: _ID_ ':'				{ $$ = $1; }		/* Explicit ALU instruction label */
			| ':'					{ $$ = NULL; }		/* Anonymous ALU instruction */

/* Allow both named and anonymous TFs inside of ALU */
/* These TFs are not separate objects, like floating TFs, but use the outer ALU resources */
alu_tf_decl		: _TF_ _ID_
				'{' tf_stmts '}'
			| _TF_
				'{' tf_stmts '}'


/* Floating TF */
tf_decl			: _TF_ _ID_				{ startObject("TF", $2); }
				'{' tf_stmts '}'		{ endObject(); }


/* TF statements are common among ALU TF and floating TF */
tf_stmts		:	/* empty */
			| tf_stmts tf_stmt


tf_stmt			: common_module_stmt
			| reg_decl
			| const_decl
			| tf_init_decl
			| warm_reset_stmt

tf_init_decl		: _INIT_
				'{' tf_init_stmts '}'


tf_init_stmts		:	/* empty */
			| tf_init_stmts tf_init_stmt


tf_init_stmt		: common_stmt
			| warm_reset_stmt
			| id_list '=' expr ';'			{	/* Initial values for registers */
									regInitialValues($1,$3);
									$1.Delete();
									$3.Delete();
								}


/* Simple objects with no special internal syntax */

simple_object_decl	: _ID_ _ID_				{ startObject($1, $2); }
				'{' common_module_stmts '}'	{ endObject(); }




/*
 *	Signal Expressions Used In Arrow Syntax
 */

left_signal_expr
	: signal_src					{	/* Base case of signal expression to the left */
								$$ = $1;
							}

	| dotted_id _LARROW_ left_signal_expr		{	/* Local connection through another signal to the left */
								$$ = SignalReference::FromId($1, DIR_NONE);
								connect($3, $$);
							}

	| _DELAY_ '(' expr ')' _LARROW_ left_signal_expr
							{	/* Through a delay to the left */
								$$ = $6;
								$$.DelayCount += $3.ToInt();
							}


right_signal_expr
	: dotted_id_list				{	/* Base case of signal expression to the right */
								$$ = SignalReferenceList::FromIdList($1, DIR_NONE);
							}

	| dotted_id _RARROW_ right_signal_expr		{	/* Local connection through another signal to the right */
								$$ = SignalReferenceList::FromId($1, DIR_NONE);
								connect($$, $3);
								$3.Delete();
							}

	| _DELAY_ '(' expr ')' _RARROW_ right_signal_expr
							{	/* Through a delay to the right */
								$$ = $6;
								$$.DelayCount += $3.ToInt();
							}


signal_src
	: dotted_id					{	/* Identifier */
								$$ = SignalReference::FromId($1, DIR_NONE);
							}

	| dotted_id _DOT_V_				{	/* V-bit slice in structural connection */
								$$ = SignalReference::FromId($1, DIR_NONE);
								$$.VBit = true;
							}


/*
 *     Function Calls (0-4 arguments currently supported)
 */

fcn_call
	: _ID_ '(' ')'					{	/* Zero-argument function-call */
								$$ = Function::Call(symbols, $1, 0, NULL, NULL, NULL, NULL);
							}

	| _ID_ '(' expr ')'				{	/* One-argument function-call */
								$$ = Function::Call(symbols, $1, 1, &$3, NULL, NULL, NULL);
								$3.Delete();
							}

	| _ID_ '(' expr ',' expr ')'			{	/* Two-argument function-call */
								$$ = Function::Call(symbols, $1, 2, &$3, &$5, NULL, NULL);
								$3.Delete();
								$5.Delete();
							}

	| _ID_ '(' expr ',' expr ',' expr ')'		{	/* Three-argument function-call */
								$$ = Function::Call(symbols, $1, 3, &$3, &$5, &$7, NULL);
								$3.Delete();
								$5.Delete();
								$7.Delete();
							}

	| _ID_ '(' expr ',' expr ',' expr ',' expr ')'	{	/* Four-argument function-call */
								$$ = Function::Call(symbols, $1, 4, &$3, &$5, &$7, &$9);
								$3.Delete();
								$5.Delete();
								$7.Delete();
								$9.Delete();
							}


/*
 *     List of Expressions in Array Initializer
 */

array_expr
	: expr						{	/* Single item array */
								$$ = Expression::Array($1);
								$1.Delete();
							}
	| array_expr ',' expr				{	/* Collect each item in turn */
								$1.val.array->AddValue($3);
								$3.Delete();
								$$ = $1;
							}

/*
 *	Expressions
 */

expr	: fcn_call

	| _VALUE_					{	/* Literal value */
								$$ = $1;
							}

	| '(' expr ')'					{	/* Grouping parentheses */
								$$ = $2;
								/* "expr" not deleted, simply copied */
							}

	| '{' '}'					{	/* Empty array */
								$$ = Expression::Array();
							}

	| '{' array_expr '}'				{	/* Non-empty array */
								$$ = $2;
							}

	| '{' array_expr ',' '}'			{	/* Non-empty array (with trailing comma) */
								$$ = $2;
							}

	| _ID_						{	/* Symbol */
								Symbol *s = symbols->Get($1);
								if (!s)
								{
									$$.type = EXPRESSION_UNKNOWN;
									yyerrorf("Symbol '%s' not defined", $1);
								}
								else if (s->SymbolType() == SYMBOL_VARIABLE)
								{
									Variable *var = (Variable *) s;

									if (var->Value.type == EXPRESSION_SIGNAL)
										checkLocalSignal(var->Value.val.sig);

									$$ = var->Value;
									$$.IncRef();
								}
								else if (s->SymbolType() == SYMBOL_SIGNAL)
								{
									Signal *sig = (Signal *) s;
									checkLocalSignal(sig);
									$$ = Expression::FromSignal(sig);
								}
								else if (s->SymbolType() == SYMBOL_ENUM_VALUE)
								{
									EnumValue *ev = (EnumValue *) s;
									$$ = ev->ToExpression();
								}
								else if (s->SymbolType() == SYMBOL_MODULE)
								{
									$$ = Expression::Unknown();
									yyerrorf("Cannot use a module name on the right-hand side of an expression: '%s'", $1);
								}
								else
								{
									$$ = Expression::Unknown();
									yyerrorf("Unknown type for variable: '%s'", $1);
								}
							}

	| _DELAY_ '(' expr ',' expr ')'			{	/* Delay of a signal within an expression */
								$$ = $3.Delay($5);
								$3.Delete();
								$5.Delete();
							}

	| expr _DOT_V_					{	/* Bit-slice of signal to obtain v-bit */
								$$ = $1.VBit();
								$1.Delete();
							}

	| expr '[' expr ']'				{	/* Array element dereference or bit-slice */
								$$ = $1[$3.ToInt()];
								$1.Delete();
								$3.Delete();
							}

	| '-' expr %prec _UNARY_MINUS_			{	/* Unary minus */
								$$ = -$2;
								$2.Delete();
							}

	| '!' expr					{	/* Unary logical NOT */
								$$ = !$2;
								$2.Delete();
							}

	| '~' expr					{	/* Unary bitwise NOT */
								$$ = ~$2;
								$2.Delete();
							}

	| expr '*' expr					{	/* Integer multiplication */
								$$ = $1 * $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '/' expr					{	/* Integer division */
								$$ = $1 / $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '%' expr					{	/* Integer modulus */
								$$ = $1 % $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '+' expr					{	/* Integer addition, string concatenation, and array append */
								$$ = $1 + $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '-' expr					{	/* Integer subtraction */
								$$ = $1 - $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _LSHIFT_ expr				{	/* Integer left shift */
								$$ = $1 << $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _RSHIFT_ expr				{	/* Integer right shift */
								$$ = $1 >> $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '<' expr					{	/* Less-than comparison */
								$$ = $1 < $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '>' expr					{	/* Greater-than comparison */
								$$ = $1 > $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _LE_ expr				{	/* Less-than-or-equal-to comparison */
								$$ = $1 <= $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _GE_ expr				{	/* Greater-than-or-equal-to comparison */
								$$ = $1 >= $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _EQ_ expr				{	/* Equal-to comparison */
								$$ = $1 == $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _NE_ expr				{	/* Not-equal-to comparison */
								$$ = $1 != $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '&' expr					{	/* Bitwise AND */
								$$ = $1 & $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '^' expr					{	/* Bitwise XOR */
								$$ = $1 ^ $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '|' expr					{	/* Bitwise OR */
								$$ = $1 | $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _AND_ expr				{	/* Logical AND */
								$$ = $1 && $3;
								$1.Delete();
								$3.Delete();
							}

	| expr _OR_ expr				{	/* Logical OR */
								$$ = $1 || $3;
								$1.Delete();
								$3.Delete();
							}

	| expr '?' expr ':' expr 
							{	/* Ternary operator */
								$$ = $1.Ternary($3,$5);
								$1.Delete();
								$3.Delete();
								$5.Delete();
							}

%%

