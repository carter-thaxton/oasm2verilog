
%{
#include "parser.h"
static long hextoi(const char *str);
static long bintoi(const char *str);
%}

%option noyywrap
%option never-interactive

%x EMBEDDED_OASM EMBEDDED_OASM_SECTION MULTI_LINE_COMMENT STRING_LITERAL

%%
	{
		switch (parseModeStart)		/* Setup state depending on expected file type */
		{
			case PARSE_OASM:		BEGIN(INITIAL);		break;
			case PARSE_EMBEDDED_OASM:	BEGIN(EMBEDDED_OASM);	break;
			default:			/* Do nothing */	break;
		}
		parseModeStart = PARSE_UNKNOWN;
	}


<EMBEDDED_OASM>{				/* Skip everything until a special comment "+++EMBEDDED_OASM+++" in found in file containing embedded OASM */
	\n					{ yylloc.first_line++; }
	"//"[ \t]*"+++EMBEDDED_OASM+++"		{ BEGIN(EMBEDDED_OASM_SECTION); }
	.
}

<EMBEDDED_OASM_SECTION>{			/* Strip single-line comments "//" and parse each line as typical OASM code until "+++END_EMBEDDED_OASM+++" */
	\n					{ yylloc.first_line++; }
	[ \t\r]+
	"//"[ \t]*"+++END_EMBEDDED_OASM+++"	{ BEGIN(EMBEDDED_OASM); }
	"//"					{ BEGIN(INITIAL); }
	<<EOF>>					{ yyerror("Unterminated embedded OASM code"); }
}




[ \t\r]+		/* Whitespace */

\n			{													/* Newline */
				/* Keep track of line number */
				yylloc.first_line++;

				/* When parsing embedded OASM, re-enter hot-comment */
				if (parseMode == PARSE_EMBEDDED_OASM)
				{
					BEGIN(EMBEDDED_OASM_SECTION);
				}
			}


"/*"			{ BEGIN(MULTI_LINE_COMMENT); }										/* Multi-line comments */

<MULTI_LINE_COMMENT>{
	[^*\n]
	"*"[^*/\n]*
	\n		{ yylloc.first_line++; }
	"*"+"/"		{ BEGIN(INITIAL); }
	<<EOF>>		{ yyerror("Unterminated comment"); }
}


"//".*																/* Single-line Comments */

'[^'\n]'		{ yylval.expr.val.i = (int) *yytext;		yylval.expr.type = CONST_INT;  return _VALUE_; }	/* Character literal */
'\\n'			{ yylval.expr.val.i = '\n';			yylval.expr.type = CONST_INT;  return _VALUE_; }
'\\r'			{ yylval.expr.val.i = '\r';			yylval.expr.type = CONST_INT;  return _VALUE_; }
'\\t'			{ yylval.expr.val.i = '\t';			yylval.expr.type = CONST_INT;  return _VALUE_; }
'\''			{ yylval.expr.val.i = '\'';			yylval.expr.type = CONST_INT;  return _VALUE_; }

[0-9]+			{ yylval.expr.val.i = atoi(yytext);		yylval.expr.type = CONST_INT;  return _VALUE_; }	/* Integer literal */
0x[0-9a-fA-F_]+		{ yylval.expr.val.i = hextoi(&yytext[2]);	yylval.expr.type = CONST_INT;  return _VALUE_; }
0b[01_]+		{ yylval.expr.val.i = bintoi(&yytext[2]);	yylval.expr.type = CONST_INT;  return _VALUE_; }

"true"			{ yylval.expr.val.i = 1;			yylval.expr.type = CONST_INT;  return _VALUE_; }	/* Boolean literal */
"false"			{ yylval.expr.val.i = 0;			yylval.expr.type = CONST_INT;  return _VALUE_; }

\"			{ BEGIN(STRING_LITERAL); strings->StartString(); }							/* String literal */

<STRING_LITERAL>{
	\"		{
				BEGIN(INITIAL);
				yylval.expr.val.s = strings->FinishString();
				yylval.expr.type = CONST_STRING;
				return _VALUE_;
			}
	\n		{ yyerror("Unterminated string"); yylloc.first_line++; }
	\\n		{ strings->AppendChar('\n'); }
	\\t		{ strings->AppendChar('\t'); }
	\\r		{ strings->AppendChar('\r'); }
	\\b		{ strings->AppendChar('\b'); }
	\\f		{ strings->AppendChar('\f'); }
	\\[0-7]{1,3}	{						/* Octal escape sequence */
				int result;
				sscanf(&yytext[1], "%o", &result);
				if (result > 0xFF) yyerrorf("Bad octal escape sequence in string: %s", yytext);
				strings->AppendChar(result);
			}
	\\[0-9]+	{ yyerrorf("Bad octal escape sequence in string: %s", yytext); }
	\\(.|\n)	{ strings->AppendChar(yytext[1]); }
	[^\\\n\"]+	{ strings->AppendString(yytext); }
}

"include"		{ return _INCLUDE_; }											/* Keywords */
"namespace"		{ return _NAMESPACE_; }
"module"		{ return _MODULE_; }
"extern"		{ return _EXTERN_; }

"ALU"			{ return _ALU_; }
"TF"			{ return _TF_; }
"FPOA"			{ return _FPOA_; }

"bit"			{ return _BIT_; }
"word"			{ return _WORD_; }
"input"			{ return _INPUT_; }
"output"		{ return _OUTPUT_; }
"reg"			{ return _REG_; }
"wire"			{ return _WIRE_; }
"const"			{ return _CONST_; }
"var"			{ return _VAR_; }
"delay"			{ return _DELAY_; }
"v"			{ return _V_; }

"if"			{ return _IF_; }
"else"			{ return _ELSE_; }
"goto"			{ return _GOTO_; }
"case"			{ return _CASE_; }

"init"			{ return _INIT_; }
"tfa"			{ return _TFA_; }
"inst"			{ return _INST_; }

"branch"		{ return _BRANCH_; }
"cond_bypass"		{ return _COND_BYPASS_; }
"cond_update"		{ return _COND_UPDATE_; }
"cond_update_vr"	{ return _COND_UPDATE_VR_; }
"cond_update_tf"	{ return _COND_UPDATE_TF_; }
"wait_for_v"		{ return _WAIT_FOR_V_; }
"warm_reset_affects"	{ return _WARM_RESET_AFFECTS_; }
"latch"			{ return _LATCH_; }

"print"			{ return _PRINT_; }						/* DEBUG */

[a-zA-Z_][a-zA-Z_0-9]*	{ yylval.str = strings->AddString(yytext);  return _ID_; }	/* Any other identifier */


"<="			{ return _LE_; }						/* Multi-character operators */
">="			{ return _GE_; }
"=="			{ return _EQ_; }
"!="			{ return _NE_; }
"&&"			{ return _AND_; }
"||"			{ return _OR_; }
"<<"			{ return _LSHIFT_; }
">>"			{ return _RSHIFT_; }
"<-"			{ return _LARROW_; }
"->"			{ return _RARROW_; }

"."[\t\r ]*"v"		{ return _DOT_V_; }						/* A little help from the lexer to lookahead past the dot to a v */
"."\n"v"		{ yylloc.first_line++; return _DOT_V_; }

[+\-*/^%&|]	|									/* Single-character operators and punctuation */
[.,:;?!~=]	|
[(){}<>\[\]]		{ return *yytext; }



%%

/*
 * Conversions for integer literals
 */
static long hextoi(const char *str)
{
	long result = 0;
	while (char c = *str) {
		int digit;
		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'A' && c <= 'F')
			digit = 10 + c - 'A';
		else if (c >= 'a' && c <= 'f')
			digit = 10 + c - 'a';
		else if (c == '_')
		{
			str++;		// skip underscores
			continue;
		}
		else
			return 0;	// unknown digit
		
		result = 16 * result + digit;
		str++;
	}
	return result;
}

static long bintoi(const char *str)
{
	long result = 0;
	while (char c = *str) {
		int digit;
		if (c >= '0' && c <= '1')
			digit = c - '0';
		else if (c == '_')
		{
			str++;		// skip underscores
			continue;
		}
		else
			return 0;	// unknown digit
		
		result = 2 * result + digit;
		str++;
	}
	return result;
}

