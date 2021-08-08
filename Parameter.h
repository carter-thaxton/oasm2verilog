
#ifndef PARAMETER_H
#define PARAMETER_H

#include "Symbol.h"
#include "Expression.h"
#include "Common.h"

#define NO_MINIMUM_VALUE    (-2147483647)
#define NO_MAXIMUM_VALUE     (2147483647)

// Similar to ExpressionType, but used to indicate the accepted data types of a parameter
enum ParameterType
{
	PARAM_ANY,
	PARAM_INT,
	PARAM_STRING,
	PARAM_ENUM,
	PARAM_ENUM_INT,
	PARAM_ENUM_OR_INT,
	PARAM_ARRAY_INT,
	PARAM_ARRAY_STRING,
};

class ParameterDefinition : public Symbol
{
public:

	// default  (int)
	ParameterDefinition(const char *name, ParameterType dataType = PARAM_INT, const Expression &defaultValue = Expression::FromInt(0), long intMask = 0xFFFF);

	// int
	ParameterDefinition(const char *name, long minValue, long maxValue, long defaultIntValue, long intMask = 0xFFFF);

	// string
	ParameterDefinition(const char *name, const char *defaultString);

	// enum
	ParameterDefinition(const char *name, const char *enumValues[], int numEnumValues, const char *defaultEnumValueString);

	// enum ints  (one of a set of ints)
	ParameterDefinition(const char *name, long intValues[], int numIntValues, long defaultIntValue);

	// enum or int  (either an enum or a ranged int)
	ParameterDefinition(const char *name, const char *enumValues[], int numEnumValues, long minValue, long maxValue, const Expression &defaultValue, long intMask = 0xFFFF);

	// int array
	ParameterDefinition(const char *name, long minValue, long maxValue, int minSize, int maxSize, long intMask = 0xFFFF);

	// string array
	ParameterDefinition(const char *name, int minSize, int maxSize);

	virtual ~ParameterDefinition();

	virtual SymbolTypeId SymbolType() const { return SYMBOL_PARAMETER; }
	virtual void Print(FILE *f) const;

	ParameterType DataType;

	long MinIntegerValue;
	long MaxIntegerValue;

	const char **EnumValues;
	long *IntValues;
	int NumEnumValues;

	int MinArraySize;
	int MaxArraySize;

	long IntegerMask;

	Expression DefaultValue;

private:
	void InitEnumValues(int count, const char *values[]);
	void InitIntValues(int count, long values[]);
};


class Parameter
{
public:
	Parameter(const ParameterDefinition *definition);
	Parameter(const ParameterDefinition *definition, const Expression &expr);
	virtual ~Parameter();

	void Init();
	void Print(FILE *f) const;
	const char *Name() const;

	bool SetValue(const Expression &expr);

	void GenerateVerilog(FILE *f) const;

	const ParameterDefinition *Definition;
	bool Assigned;
	Expression Value;

	SourceCodeLocation Location;
};


#endif
