
#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "TruthFunction.h"
#include "AluFunctionCall.h"
#include <stdio.h>

#include <vector>
using namespace::std;


class Signal;
struct ExpressionArray;


/* Expression Types */
enum ExpressionType
{
	EXPRESSION_UNKNOWN,
	CONST_INT,
	CONST_STRING,
	CONST_ENUM,
	EXPRESSION_SIGNAL,
	EXPRESSION_TF,
	EXPRESSION_ALU_FCN,
	EXPRESSION_ARRAY,
};


struct Expression
{
	ExpressionType type;
	union
	{
		long i;
		const char *s;
		Signal *sig;
		TruthFunction tf;
		AluFunctionCall fcn;
		ExpressionArray *array;
	} val;

	// Default constructor is implicitly defined
	// Because Expression is used in the yylval union, it is in fact illegal to define a constructor

	// Deletes any associated resources.
	// Specifically, when the expression is an array, it decrements the reference count, which will self-delete upon reaching zero.
	void Delete();

	// Increments the array reference count
	void IncRef() const;        // marked as const, because reference counting is done with mutables

	// Makes a shallow copy of the expression.
	Expression Copy() const;

	// Factories for conversion to Expression
	static Expression FromInt(long i);
	static Expression FromString(const char *s);
	static Expression FromEnum(const char *s);
	static Expression FromSignal(Signal *sig);
	static Expression FromTF(const TruthFunction &tf);
	static Expression FromAluFcn(const AluFunctionCall &fcn);
	static Expression Unknown();

	// Create array expression, with zero, one, or n elements
	// Further elements are obtained by appending to the underlying array
	static Expression Array();
	static Expression Array(const Expression &val);
	static Expression Array(int initial_size);

	// Conversions from Expression
	TruthFunction ToTF() const;
	Signal *ToSignal() const;
	int ToInt() const;

	// Print for Debug
	void Print(FILE *f) const;

	// Length property
	// Unknown - 0
	// Scalars - 1
	// String  - string length
	// Array   - array length
	int Length() const;

	// LHS assignment to array
	bool SetArrayValue(int index, const Expression &rhs);

	// Delay of signal within expression
	Expression Delay(const Expression &delay) const;

	// For signals, obtains a bit-slice of the v-bit
	Expression VBit() const;

	// Operators
	Expression operator[](int index) const;             // RHS array index, or bit-slice of a word

	Expression operator-() const;						// Unary minus
	Expression operator!() const;						// Unary logical NOT
	Expression operator~() const;						// Unary bitwise NOT

	Expression operator*(const Expression &) const;		// Multiplication
	Expression operator/(const Expression &) const;		// Division
	Expression operator%(const Expression &) const;		// Modulus

	Expression operator+(const Expression &) const;		// Addition
	Expression operator-(const Expression &) const;		// Subtraction

	Expression operator<<(const Expression &) const;	// Left Shift
	Expression operator>>(const Expression &) const;	// Right Shift

	Expression operator<(const Expression &) const;		// Less Than
	Expression operator>(const Expression &) const;		// Greater Than
	Expression operator<=(const Expression &) const;	// Less Than or Equal
	Expression operator>=(const Expression &) const;	// Greater Than or Equal
	Expression operator==(const Expression &) const;	// Equal
	Expression operator!=(const Expression &) const;	// Not Equal

	Expression operator&(const Expression &) const;		// Bitwise AND
	Expression operator^(const Expression &) const;		// Bitwise XOR
	Expression operator|(const Expression &) const;		// Bitwise OR

	Expression operator&&(const Expression &) const;	// Logical AND
	Expression operator||(const Expression &) const;	// Logical OR

	Expression Ternary(const Expression &op1, const Expression &op2) const;     // Ternary operator  (a ? b : c)
};

// Array of expressions, managed with a reference count
struct ExpressionArray
{
	// Constructor creates an array with optional initial_size
	ExpressionArray(int initial_size = 0);

	// Copy constructor creates a shallow copy
	ExpressionArray(const ExpressionArray &expr);

	// Appends a value to the array.  Used during array construction while parsing.
	// This also increments the RefCount of the value, because it is held in a new data structure.
	// It is decremented when this array is deleted.
	void AddValue(const Expression &value);

	// Get value from the array
	Expression GetValue(int index) const;

	// Set value in the array.  Returns false if index is out bounds, and true if value was set.
	bool SetValue(int index, const Expression &value);
	
	// Returns the number of elements in the array
	int Count() const;

	void Print(FILE *f) const;

	// Manage reference counts
	void IncRef() const;    // Marked as const to allow calls from const Expressions.  Ok because reference counting is done with mutables
	bool DecRef();          // Deletes self when decremented to zero

	// Total reference count across all ExpressionArrays maintained for debugging and assertions
	static int TotalRefCount;

	// Get the current reference count for debug
	int RefCount() const;

private:
	// The values in the array
	vector<Expression> values;

	// Reference count maintained per ExpressionArray.  Used to self-delete when it reaches zero.
	mutable int refCount;

	// Used from IncRef and DecRef to prevent infinite recursion
	// into an array containing a reference to itself, directly or indirectly
	mutable bool recursionGuard;
};

#endif
