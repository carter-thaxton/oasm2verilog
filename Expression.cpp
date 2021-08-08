
#include "Expression.h"
#include "Signal.h"
#include "Common.h"
#include "parser.h"
#include <stdio.h>

//
// Static conversion functions
//
Expression Expression::FromInt(long i)
{
	Expression expr;
	expr.type = CONST_INT;
	expr.val.i = i;
	return expr;
}

Expression Expression::FromEnum(const char *s)
{
	Expression expr;
	expr.type = CONST_ENUM;
	expr.val.s = s;
	return expr;
}

Expression Expression::FromString(const char *s)
{
	Expression expr;
	expr.type = CONST_STRING;
	expr.val.s = s;
	return expr;
}

Expression Expression::FromSignal(Signal *sig)
{
	if (sig)
	{
		Expression expr;
		expr.type = EXPRESSION_SIGNAL;
		expr.val.sig = sig;
		return expr;
	}
	return Unknown();
}

Expression Expression::FromTF(const TruthFunction &tf)
{
	Expression expr;
	expr.type = EXPRESSION_TF;
	expr.val.tf = tf;
	return expr;
}

Expression Expression::FromAluFcn(const AluFunctionCall &fcn)
{
	Expression expr;
	expr.type = EXPRESSION_ALU_FCN;
	expr.val.fcn = fcn;
	return expr;
}

Expression Expression::Unknown()
{
	Expression expr;
	expr.type = EXPRESSION_UNKNOWN;
	return expr;
}

TruthFunction Expression::ToTF() const
{
	if (type == EXPRESSION_TF)
	{
		return val.tf;
	}
	else if (type == EXPRESSION_SIGNAL)
	{
		if (val.sig->DataType != DATA_TYPE_BIT)
		{
			yyerrorf("'%s' is not declared as a bit.  Only bits may be used in truth functions", val.sig->Name());
			return TruthFunction::False();
		}
		return TruthFunction::FromSignal(val.sig);
	}
	else if (type == CONST_INT)
	{
		return TruthFunction::FromInt(val.i);
	}
	else if (type == CONST_STRING)
	{
		yyerrorf("Cannot use string in truth function");
		return TruthFunction::False();
	}
	else if (type == CONST_ENUM)
	{
		yyerrorf("Cannot use enum value in truth function");
		return TruthFunction::False();
	}
	else
	{
		yyerrorf("Cannot convert to truth function");
		return TruthFunction::False();
	}
}

Signal *Expression::ToSignal() const
{
	if (type == EXPRESSION_SIGNAL)
	{
		return val.sig;
	}
	else if (type == CONST_INT)
	{
		// This is a funny dependency.  If the expression needs to be converted to a signal,
		// then it needs to add an anonymous constant signal to the current module.
		// addSignal will return an already-created signal if a constant with the same value already exists.
		return addSignal(NULL, BEHAVIOR_CONST, DATA_TYPE_WORD, DIR_NONE, *this);
	}
	else if (type == CONST_STRING)
	{
		yyerrorf("Cannot use string as signal");
		return NULL;
	}
	else if (type == CONST_ENUM)
	{
		yyerrorf("Cannot use enum value as signal");
		return NULL;
	}
	else
	{
		yyerrorf("Cannot convert to signal");
		return NULL;
	}
}

int Expression::ToInt() const
{
	if (type == CONST_INT)
	{
		return val.i;
	}
	else if (type == EXPRESSION_SIGNAL)
	{
		if (val.sig->Behavior == BEHAVIOR_CONST)
			return val.sig->InitialValue;

		yyerrorf("Cannot use non-constant signal as integer value");
		return 0;
	}
	else
	{
		yyerrorf("Cannot convert to integer");
		return 0;
	}
}

// Create zero-length array
Expression Expression::Array()
{
	Expression expr;
	expr.type = EXPRESSION_ARRAY;
	expr.val.array = new ExpressionArray(0);
	return expr;
}

// Create array with one element
Expression Expression::Array(const Expression &val)
{
	Expression expr;
	expr.type = EXPRESSION_ARRAY;
	expr.val.array = new ExpressionArray(0);
	expr.val.array->AddValue(val);
	return expr;
}

// Create array with initial_size
// All values are initially unknown
Expression Expression::Array(int initial_size)
{
	Expression expr;
	expr.type = EXPRESSION_ARRAY;
	expr.val.array = new ExpressionArray(initial_size);
	return expr;
}

// Prints the values of the expression
void Expression::Print(FILE *f) const
{
	switch (type)
	{
		case CONST_INT:
			fprintf(f, "%ld", val.i);
			break;

		case CONST_STRING:
			fprintf(f, "%s", val.s);
			break;

		case CONST_ENUM:
			fprintf(f, "%s", val.s);
			break;

		case EXPRESSION_SIGNAL:
			val.sig->Print(f);
			break;

		case EXPRESSION_TF:
			val.tf.Print(f);
			break;

		case EXPRESSION_ARRAY:
			val.array->Print(f);
			break;

		default:
			fprintf(f, "(Unknown)");
			break;
	}
}

// Deletes any associated resources.
// Specifically, when the expression is an array, it decrements the reference count,
// which will self-delete upon reaching zero.
void Expression::Delete()
{
	if (type == EXPRESSION_ARRAY && val.array)
	{
		// Decrement array reference count, which will self-delete when zero
		// This will also delete all items in the array when it self-deletes
		val.array->DecRef();
		val.array = NULL;
	}
}

// Indicates a new reference to the expression
// Specifically, when the expression is an array, it increments the reference count.
void Expression::IncRef() const
{
	if (type == EXPRESSION_ARRAY && val.array)
	{
		// increment array reference count
		val.array->IncRef();
	}
}

// Makes a shallow copy of the expression value.
// Most types are value types and copying has no effect other than to return this expression,
// but reference types are explicitly copied with this method.
Expression Expression::Copy() const
{
	if (type == EXPRESSION_ARRAY)
	{
		Expression result;
		result.type = EXPRESSION_ARRAY;
		result.val.array = new ExpressionArray(*val.array);
		return result;
	}

	// For value types, simply copy this
	return *this;
}


// Length property
// Unknown - 0
// Scalars - 1
// String  - string length
// Array   - array length
int Expression::Length() const
{
	switch (type)
	{
		// Unknown
		case EXPRESSION_UNKNOWN:        return 0;

		// String length
		case CONST_STRING:              return strlen(val.s);

		// Array length
		case EXPRESSION_ARRAY:          return val.array->Count();

		// All other types are scalar
		default:                        return 1;
	}
}


// LHS assignment to array
bool Expression::SetArrayValue(int index, const Expression &rhs)
{
	if (type != EXPRESSION_ARRAY)
	{
		yyerrorf("Cannot use array assignment on data types other an array");
		return false;
	}

	return val.array->SetValue(index, rhs);
}


// Delay of signal within expression
Expression Expression::Delay(const Expression &delay) const
{
	if (type != EXPRESSION_SIGNAL)
	{
		yyerrorf("Cannot delay anything except a signal");
		return Unknown();
	}

	// Get a delayed version of this signal
	int delayVal = delay.ToInt();
	return FromSignal(val.sig->Delay(delayVal));
}


// Bit-slices the v-bit out of a word signal
Expression Expression::VBit() const
{
	if (type == EXPRESSION_SIGNAL)
	{
		Signal *sig = val.sig;
		Signal *result = sig->VBit();

		if (result)
			return FromSignal(result);
		else
			return Unknown();
	}

	yyerrorf("Illegal use of .v operator.  This is used only to access the v-bit of a word");
	return Unknown();
}


// RHS array index, or bit-slice of word
Expression Expression::operator[](int index) const
{
	if (type == EXPRESSION_ARRAY)
	{
		return val.array->GetValue(index);
	}
	else if (type == EXPRESSION_SIGNAL)
	{
		Signal *sig = val.sig;
		Signal *result = sig->BitSlice(index);

		if (result)
			return FromSignal(result);
		else
			return Unknown();
	}

	yyerrorf("Illegal use of [] operator.  This is used only for array references and bit-slices of a word");
	return Unknown();
}


// Unary minus
Expression Expression::operator-() const
{
	if (type == CONST_INT)
	{
		return FromInt(-val.i);
	}
	else
	{
		return Unknown();
	}
}


// Unary logical NOT
Expression Expression::operator!() const
{
	if (type == CONST_INT)
	{
		return FromInt(val.i == 0);
	}
	else if (type == EXPRESSION_SIGNAL || type == EXPRESSION_TF)
	{
		return FromTF(ToTF().NOT_TF());
	}
	else
	{
		return Unknown();
	}
}


// Unary bitwise NOT
Expression Expression::operator~() const
{
	if (type == CONST_INT)
	{
		return FromInt(~val.i);
	}
	else if (type == EXPRESSION_SIGNAL || type == EXPRESSION_TF)
	{
		return FromTF(ToTF().NOT_TF());
	}
	else
	{
		return Unknown();
	}
}

// Multiplication
Expression Expression::operator*(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i * op.val.i);
	}
	else
	{
		return Unknown();
	}
}


// Division
Expression Expression::operator/(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		if (op.val.i == 0)
		{
			yyerror("Division by zero");
			return Unknown();
		}
		else
		{
			return FromInt(val.i / op.val.i);
		}
	}
	else
	{
		return Unknown();
	}
}


// Modulus
Expression Expression::operator%(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		if (op.val.i == 0)
		{
			yyerror("Modulus by zero");
			return Unknown();
		}
		else
		{
			return FromInt(val.i % op.val.i);
		}
	}
	else
	{
		return Unknown();
	}
}


// Addition and string concatenation
Expression Expression::operator+(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		// Integer addition
		return FromInt(val.i + op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		// String concatenation
		strings->StartString();
		strings->AppendString(val.s);
		strings->AppendString(op.val.s);
		return FromString(strings->FinishString());
	}
	else if (type == CONST_STRING && op.type == CONST_INT)
	{
		// Append integer to string
		char buf[80];
		sprintf(buf, "%ld", op.val.i);

		strings->StartString();
		strings->AppendString(val.s);
		strings->AppendString(buf);
		return FromString(strings->FinishString());
	}
	else if (type == CONST_INT && op.type == CONST_STRING)
	{
		// Convert integer to string and append string
		char buf[80];
		sprintf(buf, "%ld", val.i);

		strings->StartString();
		strings->AppendString(buf);
		strings->AppendString(op.val.s);
		return FromString(strings->FinishString());
	}
	else
	{
		return Unknown();
	}
}


// Subtraction
Expression Expression::operator-(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		// Integer subtraction
		return FromInt(val.i - op.val.i);
	}
	else
	{
		return Unknown();
	}
}


// Left Shift
Expression Expression::operator<<(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i << op.val.i);
	}
	else
	{
		return Unknown();
	}
}


// Right Shift
Expression Expression::operator>>(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i >> op.val.i);
	}
	else
	{
		return Unknown();
	}
}


// Less Than
Expression Expression::operator<(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i < op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) < 0);
	}
	else
	{
		return Unknown();
	}
}


// Greater Than
Expression Expression::operator>(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i > op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) > 0);
	}
	else
	{
		return Unknown();
	}
}


// Less Than or Equal
Expression Expression::operator<=(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i <= op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) <= 0);
	}
	else
	{
		return Unknown();
	}
}


// Greater Than or Equal
Expression Expression::operator>=(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i >= op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) >= 0);
	}
	else
	{
		return Unknown();
	}
}


// Equal
Expression Expression::operator==(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i == op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) == 0);
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().EQ_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Not Equal
Expression Expression::operator!=(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i != op.val.i);
	}
	else if (type == CONST_STRING && op.type == CONST_STRING)
	{
		return FromInt(strcmp(val.s, op.val.s) != 0);
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().NE_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Bitwise AND
Expression Expression::operator&(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i & op.val.i);
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().AND_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Bitwise XOR
Expression Expression::operator^(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i ^ op.val.i);
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().XOR_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Bitwise OR
Expression Expression::operator|(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt(val.i | op.val.i);
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().OR_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Logical AND
Expression Expression::operator&&(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt((val.i != 0) && (op.val.i != 0));
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().AND_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}


// Logical OR
Expression Expression::operator||(const Expression &op) const
{
	if (type == CONST_INT && op.type == CONST_INT)
	{
		return FromInt((val.i != 0) || (op.val.i != 0));
	}
	else if ((   type == EXPRESSION_SIGNAL ||    type == EXPRESSION_TF ||    type == CONST_INT) &&
			 (op.type == EXPRESSION_SIGNAL || op.type == EXPRESSION_TF || op.type == CONST_INT))
	{
		return FromTF(ToTF().OR_TF(op.ToTF()));
	}
	else
	{
		return Unknown();
	}
}

// Ternary operator  (a ? b : c)
Expression Expression::Ternary(const Expression &op1, const Expression &op2) const
{
	if (type == CONST_INT)
	{
		if (val.i != 0)
			return op1;
		else
			return op2;
	}
	else if ((    type == EXPRESSION_SIGNAL ||     type == EXPRESSION_TF ||     type == CONST_INT) &&
			 (op1.type == EXPRESSION_SIGNAL || op1.type == EXPRESSION_TF || op1.type == CONST_INT) &&
			 (op2.type == EXPRESSION_SIGNAL || op2.type == EXPRESSION_TF || op2.type == CONST_INT))
	{
		return FromTF(ToTF().Ternary_TF(op1.ToTF(), op2.ToTF()));
	}
	else
	{
		return Unknown();
	}
}



//
// Expression Array Type
//

ExpressionArray::ExpressionArray(int initial_size)
	: values(initial_size), refCount(0), recursionGuard(false)
{
	IncRef();
}

ExpressionArray::ExpressionArray(const ExpressionArray &expr)
	: values(expr.values), refCount(0), recursionGuard(false)
{
	IncRef();

	// Increment the reference count of all contained expression values
	int n = values.size();
	for (int i=0; i < n; i++)
	{
		values[i].IncRef();
	}
}


void ExpressionArray::Print(FILE *f) const
{
	// When printing, avoid infinite recursion if the list contains itself
	// Simply print (Recursive) for an already-referenced element
	if (recursionGuard)
	{
		fprintf(f, "(Recursive)");
		return;
	}

	// Enter recursion
	recursionGuard = true;

	// Print each element
	int n = values.size();
	fprintf(f, "{");
	for (int i=0; i < n; i++)
	{
		// Potentially recursive
		values[i].Print(f);
		if (i < n-1)
			fprintf(f, ", ");
	}
	fprintf(f, "}");

	// Exit recursion
	recursionGuard = false;
}

// Append a value to the array
// Also increment the stored value reference count
void ExpressionArray::AddValue(const Expression &value)
{
	values.push_back(value);
	value.IncRef();
}

// Get and set values in the array
Expression ExpressionArray::GetValue(int index) const
{
	if (index < 0 || index >= Count())
	{
		yyerrorf("Array index out of range: %d", index);
		return Expression::Unknown();
	}

	// Increment reference count on the way out
	// It will get decremented again wherever it lands
	Expression result = values[index];
	result.IncRef();
	return result;
}

bool ExpressionArray::SetValue(int index, const Expression &value)
{
	if (index < 0)
	{
		yyerrorf("Array index out of range: %d", index);
		return false;
	}

	// If index is larger than the largest existing index,
	// resize the underlying array to contain the new index,
	// and fill in with Unknown expression values
	if (index >= Count())
	{
		values.resize(index+1, Expression::Unknown());
	}
	else
	{
		// Delete the old value.  There are no null expression values.
		// Rather, if this is an Unknown expression, this will have no effect
		values[index].Delete();
	}

	// Store the new value, and increment its reference count
	values[index] = value;
	value.IncRef();

	return true;
}

// Returns the number of elements in the array
int ExpressionArray::Count() const
{
	return (int) values.size();
}



// Increment reference count for each new reference
// Marked as const because reference counting is done with mutables
void ExpressionArray::IncRef() const
{
	// Increment refCount
	refCount++;

	// Also increment TotalRefCount, for debugging and assertions
	TotalRefCount++;

	if (yydebug)
	{
		yywarnf("Incrementing - refCount: %d  TotalRefCount: %d", refCount, TotalRefCount);
	}
}


// Self-delete ExpressionArray when refCount reaches zero
// Returns true if deleted, and false otherwise

// When self-deleting, this should also delete all indirect references via this array,
// to support arrays of arrays.  Because arrays are referred to by reference,
// it is possible to have an array that refers to itself, either directly or indirectly
// via another array.  recursionGuard is used to prevent infinite recursion.
bool ExpressionArray::DecRef()
{
	// Prevent infinite recursion
	if (recursionGuard)
		return false;

	if (yydebug)
	{
		yywarnf("Decrementing - refCount: %d  TotalRefCount: %d", refCount-1, TotalRefCount-1);
	}

	// Set guard on entry.  Release it on every exit.
	recursionGuard = true;

	// Decrement refCount
	if (refCount <= 0)
	{
		yyerrorf("Attempt to decrement array reference count below zero: %d", refCount-1);
		recursionGuard = false;
		return false;
	}
	refCount--;

	// Decrement TotalRefCount
	if (TotalRefCount <= 0)
	{
		yyerrorf("Attempt to decrement total array reference count below zero: %d", TotalRefCount-1);
	}
	else
	{
		TotalRefCount--;
	}


	// Delete self when refCount reaches zero
	if (refCount == 0)
	{
		// Decrement refCount on all referenced expressions when finally deleted
		int n = values.size();
		for (int i=0; i < n; i++)
		{
			values[i].Delete();
		}

		recursionGuard = false;

		// Delete self
		delete this;

		return true;
	}
	else
	{
		recursionGuard = false;
		return false;
	}
}

int ExpressionArray::RefCount() const
{
	return refCount;
}

// Maintain a reference count across all expression arrays
int ExpressionArray::TotalRefCount = 0;
