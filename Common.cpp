
#include "Common.h"

// Note, uses symbols from parser.h
// Define TEST_COMMON to get behavior that does not depend on the parser

#ifndef TEST_COMMON
#include "parser.h"
#endif


// Global string buffer
StringBuffer *strings = NULL;


// Return current location
SourceCodeLocation CurrentLocation()
{
	SourceCodeLocation loc;
#ifndef TEST_COMMON
	loc.Filename = currentFilename;
	loc.Line = yylloc.first_line;
#else
	loc.Filename = NULL;
	loc.Line = 0;
#endif
	return loc;
}


// Error handlers
void yyerror(const char *str)
{
	yyerrorf(str);
}

void yyerrorf(const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yyerrorfv(str, args);

	va_end(args);
}

void yyerrorfl(SourceCodeLocation loc, const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yyerrorflv(loc, str, args);

	va_end(args);
}


void yyerrorfv(const char *str, va_list args)
{
	yyerrorflv(CurrentLocation(), str, args);
}

void yyerrorflv(SourceCodeLocation loc, const char *str, va_list args)
{
	if (loc.Filename)
		fprintf(stderr, "ERROR in %s on line %d: ", loc.Filename, loc.Line);
	else
		fprintf(stderr, "ERROR on line %d: ", loc.Line);
	vfprintf(stderr, str, args);
	fprintf(stderr, "\n");

	errorCount++;
}

int errorCount = 0;


void yywarn(const char *str)
{
	yywarnf(str);
}

void yywarnf(const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yywarnfv(str, args);

	va_end(args);
}

void yywarnfl(SourceCodeLocation loc, const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yywarnflv(loc, str, args);

	va_end(args);
}


void yywarnfv(const char *str, va_list args)
{
	if (warnAsError)
	{
		yyerrorflv(CurrentLocation(), str, args);
	}
	else
	{
		yywarnflv(CurrentLocation(), str, args);
	}
}

void yywarnflv(SourceCodeLocation loc, const char *str, va_list args)
{
	if (warnAsError)
	{
		yyerrorflv(loc, str, args);
	}
	else
	{
#ifndef TEST_COMMON
		if (loc.Filename)
			fprintf(stderr, "WARNING in %s on line %d: ", loc.Filename, loc.Line);
		else
			fprintf(stderr, "WARNING on line %d: ", loc.Line);
#else
		fprintf(stderr, "WARNING: ");
#endif
		vfprintf(stderr, str, args);
		fprintf(stderr, "\n");

		warnCount++;
	}
}

int warnCount = 0;



void yyfatal(const char *str)
{
	yyfatalf(str);
}

void yyfatalf(const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yyfatalfv(str, args);

	va_end(args);
}

void yyfatalfl(SourceCodeLocation loc, const char *str, ...)
{
	va_list args;
	va_start(args, str);

	yyfatalflv(loc, str, args);

	va_end(args);
}


void yyfatalfv(const char *str, va_list args)
{
	yyfatalflv(CurrentLocation(), str, args);
}

void yyfatalflv(SourceCodeLocation loc, const char *str, va_list args)
{
#ifndef TEST_COMMON
	if (loc.Filename)
		fprintf(stderr, "FATAL in %s on line %d: ", loc.Filename, loc.Line);
	else
		fprintf(stderr, "FATAL on line %d: ", loc.Line);
#else
		fprintf(stderr, "WARNING: ");
#endif
	vfprintf(stderr, str, args);
	fprintf(stderr, "\n");

	fatalCount++;
}


int fatalCount = 0;



// helper function to concatenate onto a string in a fixed-size buffer
// writes as much as possible into the buffer, and always null-terminates the result
// returns the length of the final result, without the null-terminator
int strcatbuf(char *buf, int bufsize, const char *str)
{
	int buflen = strlen(buf);
	int len = strlen(str);

	int bufleft = bufsize - buflen - 1;
	if (bufleft <= 0)
		return bufsize-1;
	
	int catlen = (len < bufleft) ? len : bufleft;
	int newlen = buflen + catlen;

	strncpy(&buf[buflen], str, catlen);
	buf[newlen] = 0;

	return newlen;
}
