
#include "Parameter.h"
#include "Common.h"

//
// Parameter definition containing metadata across all instances of the parameter
//

// default
ParameterDefinition::ParameterDefinition(const char *name, ParameterType dataType, const Expression &defaultValue, long intMask)
	: Symbol(name), DataType(dataType), MinIntegerValue(NO_MINIMUM_VALUE), MaxIntegerValue(NO_MAXIMUM_VALUE), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(intMask), DefaultValue(defaultValue)
{
}

// int
ParameterDefinition::ParameterDefinition(const char *name, long minValue, long maxValue, long defaultIntValue, long intMask)
	: Symbol(name), DataType(PARAM_INT), MinIntegerValue(minValue), MaxIntegerValue(maxValue), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(intMask), DefaultValue(Expression::FromInt(defaultIntValue))
{
}

// string
ParameterDefinition::ParameterDefinition(const char *name, const char *defaultString)
	: Symbol(name), DataType(PARAM_STRING), MinIntegerValue(NO_MINIMUM_VALUE), MaxIntegerValue(NO_MAXIMUM_VALUE), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(-1), DefaultValue(Expression::FromString(defaultString))
{
}

// enum
ParameterDefinition::ParameterDefinition(const char *name, const char *enumValues[], int numEnumValues, const char *defaultEnumValueString)
	: Symbol(name), DataType(PARAM_ENUM), MinIntegerValue(NO_MINIMUM_VALUE), MaxIntegerValue(NO_MAXIMUM_VALUE), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(-1), DefaultValue(Expression::FromEnum(defaultEnumValueString))
{
	InitEnumValues(numEnumValues, enumValues);
}

// enum int
ParameterDefinition::ParameterDefinition(const char *name, long intValues[], int numIntValues, long defaultIntValue)
	: Symbol(name), DataType(PARAM_ENUM_INT), MinIntegerValue(NO_MINIMUM_VALUE), MaxIntegerValue(NO_MAXIMUM_VALUE), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(-1), DefaultValue(Expression::FromInt(defaultIntValue))
{
	InitIntValues(numIntValues, intValues);
}

// enum or int
ParameterDefinition::ParameterDefinition(const char *name, const char *enumValues[], int numEnumValues, long minValue, long maxValue, const Expression &defaultValue, long intMask)
	: Symbol(name), DataType(PARAM_ENUM_OR_INT), MinIntegerValue(minValue), MaxIntegerValue(maxValue), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(-1), IntegerMask(intMask), DefaultValue(defaultValue)
{
	InitEnumValues(numEnumValues, enumValues);
}

// int array
ParameterDefinition::ParameterDefinition(const char *name, long minValue, long maxValue, int minSize, int maxSize, long intMask)
	: Symbol(name), DataType(PARAM_ARRAY_INT), MinIntegerValue(minValue), MaxIntegerValue(maxValue), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(minSize), MaxArraySize(maxSize), IntegerMask(intMask), DefaultValue(Expression::Array())
{
}

// string array
ParameterDefinition::ParameterDefinition(const char *name, int minSize, int maxSize)
	: Symbol(name), DataType(PARAM_ARRAY_STRING), MinIntegerValue(NO_MINIMUM_VALUE), MaxIntegerValue(NO_MAXIMUM_VALUE), EnumValues(NULL), IntValues(NULL), NumEnumValues(0), MinArraySize(0), MaxArraySize(minSize), IntegerMask(maxSize), DefaultValue(Expression::Array())
{
}




ParameterDefinition::~ParameterDefinition()
{
	delete [] EnumValues;  EnumValues = NULL;
	delete [] IntValues;   IntValues = NULL;
	NumEnumValues = 0;
	DefaultValue.Delete();
}


void ParameterDefinition::Print(FILE *f) const
{
	if (DataType == PARAM_INT)
	{
		if (MinIntegerValue != NO_MINIMUM_VALUE && MaxIntegerValue != NO_MAXIMUM_VALUE)
			fprintf(f, "%s : int", Name());
		else
			fprintf(f, "%s : int [%ld,%ld]", Name(), MinIntegerValue, MaxIntegerValue);
	}
	else if (DataType == PARAM_ENUM)
	{
		fprintf(f, "%s : enum [", Name());
		for (int i=0; i < NumEnumValues; i++)
		{
			fprintf(f, "%s", EnumValues[i]);
			if (i < NumEnumValues-1)
				fprintf(f, ", ");
		}
		fprintf(f, "]");
	}
	else if (DataType == PARAM_ENUM_INT)
	{
		fprintf(f, "%s : int [", Name());
		for (int i=0; i < NumEnumValues; i++)
		{
			fprintf(f, "%ld", IntValues[i]);
			if (i < NumEnumValues-1)
				fprintf(f, ", ");
		}
		fprintf(f, "]");
	}
	else if (DataType == PARAM_ENUM_OR_INT)
	{
		fprintf(f, "%s : enum [", Name());
		for (int i=0; i < NumEnumValues; i++)
		{
			fprintf(f, "%s", EnumValues[i]);
			if (i < NumEnumValues-1)
				fprintf(f, ", ");
		}
		fprintf(f, "] or int [%ld,%ld]", MinIntegerValue, MaxIntegerValue);
	}
	else if (DataType == PARAM_STRING)
	{
		fprintf(f, "%s : string", Name());
	}
	else if (DataType == PARAM_ARRAY_INT)
	{
		if (MinArraySize <= 0 && MaxArraySize < 0)
		{
			fprintf(f, "%s : int[]", Name());
		}
		else if (MaxArraySize < 0)
		{
			fprintf(f, "%s : int[%d-inf]", Name(), MinArraySize);
		}
		else if (MinArraySize == MaxArraySize)
		{
			fprintf(f, "%s : int[%d]", Name(), MinArraySize);
		}
		else
		{
			fprintf(f, "%s : int[%d-%d]", Name(), MinArraySize, MaxArraySize);
		}
	}
	else if (DataType == PARAM_ARRAY_STRING)
	{
		if (MinArraySize <= 0 && MaxArraySize < 0)
		{
			fprintf(f, "%s : string[]", Name());
		}
		else if (MaxArraySize < 0)
		{
			fprintf(f, "%s : string[%d-inf]", Name(), MinArraySize);
		}
		else if (MinArraySize == MaxArraySize)
		{
			fprintf(f, "%s : string[%d]", Name(), MinArraySize);
		}
		else
		{
			fprintf(f, "%s : string[%d-%d]", Name(), MinArraySize, MaxArraySize);
		}
	}
	else
	{
		fprintf(f, "%s : UNKNOWN_TYPE", Name());
	}
}


void ParameterDefinition::InitEnumValues(int count, const char *values[])
{
	if (count <= 0)
	{
		NumEnumValues = 0;
		EnumValues = NULL;
		return;
	}
	
	EnumValues = new const char*[count];
	NumEnumValues = count;

	for (int i=0; i < count; i++)
	{
		EnumValues[i] = values[i];
	}
}

void ParameterDefinition::InitIntValues(int count, long values[])
{
	if (count <= 0)
	{
		NumEnumValues = 0;
		IntValues = NULL;
		return;
	}
	
	IntValues = new long[count];
	NumEnumValues = count;

	for (int i=0; i < count; i++)
	{
		IntValues[i] = values[i];
	}
}


//
// Parameter value in symbol table
//

Parameter::Parameter(const ParameterDefinition *definition)
	: Definition(definition), Assigned(false)
{
}

Parameter::Parameter(const ParameterDefinition *definition, const Expression &expr)
	: Definition(definition), Assigned(false)
{
	SetValue(expr);
}


Parameter::~Parameter()
{
}


void Parameter::Init()
{
	Assigned = false;
	Value = Expression::Unknown();
}

// Use definition name
const char *Parameter::Name() const
{
	if (Definition == NULL)
		return NULL;
	else
		return Definition->Name();
}

// Set Parameter via an Expression
// Perform appropriate data type and value checking
bool Parameter::SetValue(const Expression &expr)
{
	bool ok = false;

	// Check if no definition
	if (Definition == NULL)
	{
		yyerrorf("Cannot set undefined parameter");
		return false;
	}

	// Check if already assigned
	else if (Assigned)
	{
		yyerrorf("Parameter '%s' has already been assigned a value", Name());
		return false;
	}

	// Integer
	else if (Definition->DataType == PARAM_INT)
	{
		if (expr.type != CONST_INT)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);
			fprintf(stderr, "\nParameter '%s' expects an integer value", Name());
			return false;
		}
		else if (expr.val.i < Definition->MinIntegerValue || expr.val.i > Definition->MaxIntegerValue)
		{
			yyerrorf("Parameter '%s' out of range: %ld.  Must be a value from %ld to %ld", Name(), expr.val.i, Definition->MinIntegerValue, Definition->MaxIntegerValue);
			return false;
		}
		ok = true;
	}

	// Enum
	else if (Definition->DataType == PARAM_ENUM)
	{
		if (expr.type == CONST_ENUM)
		{
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				if (strcmp(expr.val.s, Definition->EnumValues[i]) == 0)
				{
					ok = true;
					break;
				}
			}
		}

		if (!ok)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);
			fprintf(stderr, "\nParameter '%s' expects one of: ", Name());
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				fprintf(stderr, "%s", Definition->EnumValues[i]);
				if (i < Definition->NumEnumValues-1)
					fprintf(stderr, ", ");
			}
			return false;
		}
	}

	// Enum Int
	else if (Definition->DataType == PARAM_ENUM_INT)
	{
		if (expr.type == CONST_INT)
		{
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				if (expr.val.i == Definition->IntValues[i])
				{
					ok = true;
					break;
				}
			}
		}

		if (!ok)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);
			fprintf(stderr, "\nParameter '%s' expects one of: ", Name());
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				fprintf(stderr, "%ld", Definition->IntValues[i]);
				if (i < Definition->NumEnumValues-1)
					fprintf(stderr, ", ");
			}
			return false;
		}
	}

	// Enum or Integer
	else if (Definition->DataType == PARAM_ENUM_OR_INT)
	{
		if (expr.type == CONST_INT)
		{
			if (expr.val.i >= Definition->MinIntegerValue && expr.val.i <= Definition->MaxIntegerValue)
			{
				ok = true;
			}
		}
		else if (expr.type == CONST_ENUM)
		{
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				if (strcmp(expr.val.s, Definition->EnumValues[i]) == 0)
				{
					ok = true;
					break;
				}
			}
		}

		if (!ok)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);
			fprintf(stderr, "\nParameter '%s' expects one of: ", Name());
			for (int i=0; i < Definition->NumEnumValues; i++)
			{
				fprintf(stderr, "%s", Definition->EnumValues[i]);
				if (i < Definition->NumEnumValues-1)
					fprintf(stderr, ", ");
			}
			fprintf(stderr, " or a value from %ld to %ld", Definition->MinIntegerValue, Definition->MaxIntegerValue);
			return false;
		}
	}

	// String
	else if (Definition->DataType == PARAM_STRING)
	{
		if (expr.type != CONST_STRING)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);
			fprintf(stderr, "\nParameter '%s' expects a string", Name());
			return false;
		}
		ok = true;
	}

	// Array of Integers
	else if (Definition->DataType == PARAM_ARRAY_INT)
	{
		if (expr.type == EXPRESSION_ARRAY)
		{
			ok = true;
			int n = expr.val.array->Count();

			// Check whether the array size is correct
			if (Definition->MinArraySize >= 0)
				if (n < Definition->MinArraySize)
					ok = false;
			if (Definition->MaxArraySize >= 0)
				if (n > Definition->MaxArraySize)
					ok = false;

			// Check whether the array contains all integers
			for (int i=0; ok && i < n; i++)
			{
				if (expr.val.array->GetValue(i).type != CONST_INT)
				{
					ok = false;
				}
			}
		}

		if (!ok)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);

			if (Definition->MinArraySize <= 0 && Definition->MaxArraySize < 0)
				fprintf(stderr, "\nParameter '%s' expects an array of integers", Name());
			else if (Definition->MaxArraySize < 0)
				fprintf(stderr, "\nParameter '%s' expects an array of at least %d integers", Name(), Definition->MinArraySize);
			else if (Definition->MinArraySize == Definition->MaxArraySize)
				fprintf(stderr, "\nParameter '%s' expects an array of %d integers", Name(), Definition->MinArraySize);
			else
				fprintf(stderr, "\nParameter '%s' expects an array of %d to %d integers", Name(), Definition->MinArraySize, Definition->MaxArraySize);

			return false;
		}
	}

	// Array of strings
	else if (Definition->DataType == PARAM_ARRAY_STRING)
	{
		if (expr.type == EXPRESSION_ARRAY)
		{
			ok = true;
			int n = expr.val.array->Count();

			// Check whether the array size is correct
			if (Definition->MinArraySize >= 0)
				if (n < Definition->MinArraySize)
					ok = false;
			if (Definition->MaxArraySize >= 0)
				if (n > Definition->MaxArraySize)
					ok = false;

			// Check whether the array contains all strings
			for (int i=0; ok && i < n; i++)
			{
				if (expr.val.array->GetValue(i).type != CONST_STRING)
				{
					ok = false;
				}
			}
		}

		if (!ok)
		{
			yyerrorf("Illegal value: ");
			expr.Print(stderr);

			if (Definition->MinArraySize <= 0 && Definition->MaxArraySize < 0)
				fprintf(stderr, "\nParameter '%s' expects an array of strings", Name());
			else if (Definition->MaxArraySize < 0)
				fprintf(stderr, "\nParameter '%s' expects an array of at least %d strings", Name(), Definition->MinArraySize);
			else if (Definition->MinArraySize == Definition->MaxArraySize)
				fprintf(stderr, "\nParameter '%s' expects an array of %d strings", Name(), Definition->MinArraySize);
			else
				fprintf(stderr, "\nParameter '%s' expects an array of %d to %d strings", Name(), Definition->MinArraySize, Definition->MaxArraySize);

			return false;
		}
	}

	// Unknown type
	else
	{
		yyerrorf("Unknown data type for parameter: '%s'", Name());
		return false;
	}

	// Set expression value, and increment its reference count, because it is being stored in a parameter
	if (ok)
	{
		Value = expr;
		Value.IncRef();
		Assigned = true;
		return true;
	}

	return false;
}


void Parameter::Print(FILE *f) const
{
	if (Definition == NULL)
	{
		fprintf(f, "UNKNOWN");
	}
	else if (Definition->DataType == PARAM_INT || Definition->DataType == PARAM_ENUM_INT)
	{
		if (!Assigned)
			fprintf(f, "%s : int = (unknown)", Name());
		else
			fprintf(f, "%s : int = %ld", Name(), Value.val.i);
	}
	else if (Definition->DataType == PARAM_ENUM)
	{
		if (!Assigned)
			fprintf(f, "%s : enum = (unknown)", Name());
		else
			fprintf(f, "%s : enum = %s", Name(), Value.val.s);
	}
	else if (Definition->DataType == PARAM_ENUM_OR_INT)
	{
		if (!Assigned)
			fprintf(f, "%s : enum or int = (unknown)", Name());
		else if (Value.type == CONST_STRING)
			fprintf(f, "%s : enum = %s", Name(), Value.val.s);
		else
			fprintf(f, "%s : int = %ld", Name(), Value.val.i);
	}
	else if (Definition->DataType == PARAM_STRING)
	{
		if (!Assigned)
			fprintf(f, "%s : string = (unknown)", Name());
		else
			fprintf(f, "%s : string = \"%s\"", Name(), Value.val.s);
	}
	else if (Definition->DataType == PARAM_ARRAY_INT)
	{
		if (!Assigned)
			fprintf(f, "%s : int[] = (unknown)", Name());
		else
		{
			fprintf(f, "%s : int[%d] = ", Name(), Value.val.array->Count());
			Value.val.array->Print(f);
		}
	}
	else if (Definition->DataType == PARAM_ARRAY_STRING)
	{
		if (!Assigned)
			fprintf(f, "%s : string[] = (unknown)", Name());
		else
		{
			fprintf(f, "%s : string[%d] = ", Name(), Value.val.array->Count());
			Value.val.array->Print(f);
		}
	}
	else
	{
		fprintf(f, "%s : UNKNOWN_TYPE", Name());
	}
}


static void PrintCaps(FILE *f, const char *str)
{
	if (str == NULL) return;

	while (*str)
	{
		fputc(toupper(*str), f);
		str++;
	}
}


// Generates a capitalized version of the parameter and value like:  \t\t.RD_WIDTH("1")
//
// Generates arrays as multiple parameters like:
//     \t\t.INIT_DATA0("123"),
//     \t\t.INIT_DATA1("456"),
//     \t\t.INIT_DATA2("789")
//
// Note that there is no comma after the last parameter (or after the only parameter when scalar)
void Parameter::GenerateVerilog(FILE *f) const
{
	if (Definition == NULL) return;

	if (Definition->DataType == PARAM_ARRAY_INT)
	{
		bool first = true;
		int n = Value.val.array->Count();
		for (int i=0; i < n; i++)
		{
			long val = Value.val.array->GetValue(i).val.i;

			if (!first)
				fprintf(f, ",\n");
			first = false;

			// \t\t.NAME0("123")
			fprintf(f, "\t\t.");
			PrintCaps(f, Name());
			fprintf(f, "%d(\"%ld\")", i, val);
		}
	}
	else if (Definition->DataType == PARAM_ARRAY_STRING)
	{
		bool first = true;
		int n = Value.val.array->Count();
		for (int i=0; i < n; i++)
		{
			const char *str = Value.val.array->GetValue(i).val.s;

			if (!first)
				fprintf(f, ",\n");
			first = false;

			// \t\t.NAME0("abc")
			fprintf(f, "\t\t.");
			PrintCaps(f, Name());
			fprintf(f, "%d(\"%s\")", i, str);
		}
	}
	else
	{
		// Scalar

		// \t\t.NAME("
		fprintf(f, "\t\t.");
		PrintCaps(f, Name());
		fprintf(f, "(\"");

		if (Definition->DataType == PARAM_INT || Definition->DataType == PARAM_ENUM_INT)
		{
			fprintf(f, "%ld", Value.val.i);
		}
		else if (Definition->DataType == PARAM_ENUM)
		{
			PrintCaps(f, Value.val.s);
		}
		else if (Definition->DataType == PARAM_ENUM_OR_INT)
		{
			if (Value.type == CONST_STRING)
				PrintCaps(f, Value.val.s);
			else
				fprintf(f, "%ld", Value.val.i);
		}
		else if (Definition->DataType == PARAM_STRING)
		{
			fprintf(f, "%s", Value.val.s);	// enums are capitalized, strings are not
		}

		// ")
		fprintf(f, "\")");
	}

}
