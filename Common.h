
#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "StringBuffer.h"

/*
 * Handy macros for code-generation
 */
#define F0(s)				fprintf(f, s)
#define F1(s,a)				fprintf(f, s, a)
#define F2(s,a,b)			fprintf(f, s, a, b)
#define F3(s,a,b,c)			fprintf(f, s, a, b, c)
#define F4(s,a,b,c,d)		fprintf(f, s, a, b, c, d)
#define F5(s,a,b,c,d,e)		fprintf(f, s, a, b, c, d, e)

/*
 * Define countof() macro that yields the length of an array
 */
#ifndef countof
#define countof(x)   (sizeof(x) / sizeof((x)[0]))
#endif

/*
 * Common type for source code location
 */
struct SourceCodeLocation
{
	const char *Filename;
	int Line;
};

// Return current location
extern SourceCodeLocation CurrentLocation();


/*
 * Dynamic string buffer used during parse
 */
extern StringBuffer *strings;

/*
 * Error handling
 */
extern void yyerror(char const *str);
extern void yyerrorf(char const *str, ...);
extern void yyerrorfv(char const *str, va_list args);
extern void yyerrorfl(SourceCodeLocation loc, const char *str, ...);
extern void yyerrorflv(SourceCodeLocation loc, const char *str, va_list args);
extern int errorCount;

extern void yywarn(char const *str);
extern void yywarnf(char const *str, ...);
extern void yywarnfv(char const *str, va_list args);
extern void yywarnfl(SourceCodeLocation loc, const char *str, ...);
extern void yywarnflv(SourceCodeLocation loc, const char *str, va_list args);
extern int warnCount;

extern void yyfatal(char const *str);
extern void yyfatalf(char const *str, ...);
extern void yyfatalfv(char const *str, va_list args);
extern void yyfatalfl(SourceCodeLocation loc, const char *str, ...);
extern void yyfatalflv(SourceCodeLocation loc, const char *str, va_list args);
extern int fatalCount;

extern bool warnAsError;

extern int yydebug;

extern const char *currentFilename;


// helper function to concatenate onto a string in a fixed-size buffer
// writes as much as possible into the buffer, and always null-terminates the result
// returns the length of the final result, without the null-terminator
extern int strcatbuf(char *buf, int bufsize, const char *str);

#endif
