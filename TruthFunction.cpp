
#include "Common.h"
#include "TruthFunction.h"
#include "Signal.h"


void TruthFunction::Init()
{
	int i;
	for (i=0; i < 4; i++)
	{
		Args[i] = NULL;
	}
	Logic = 0;
	Buffer[0] = 0;
}

void TruthFunction::Print(FILE *f) const
{
	int i;

	fprintf(f, "TF:0x%04X %s", Logic, Buffer);
	for (i=0; i < 4; i++)
	{
		if (Args[i])
		{
			fprintf(f, " %d:%s", i, Args[i]->Name());
		}
	}
}

int TruthFunction::NumArgs() const
{
	int i;
	for (i=0; i < 4; i++)
	{
		if (!Args[i])
			return i;
	}
	return i;
}

bool TruthFunction::IsFalse() const
{
	return (Logic == 0x0000);
}

bool TruthFunction::IsTrue() const
{
	return (Logic == 0xFFFF);
}

bool TruthFunction::IsHold(const Signal *signal) const
{
	return (Logic == 0xAAAA) && (Args[0] == signal) && signal;
}

int TruthFunction::AddArg(Signal *signal)
{
	if (signal == NULL || signal->DataType != DATA_TYPE_BIT)
		return -1;

	int i;
	for (i=0; i < 4; i++)
	{
		if (Args[i] == NULL)
			break;

		// Return existing signal index
		if (Args[i] == signal)
			return i;
	}

	if (i >= 4)
	{
		yyerror("Cannot create TF with more than 4 arguments");
		return -1;
	}

	Args[i] = signal;
	return i;
}

// Factories for specific truth-functions
TruthFunction TruthFunction::True()
{
	TruthFunction tf;
	tf.Init();
	tf.Logic = 0xFFFF;
	tf.Buffer[0] = 'T';
	tf.Buffer[1] = 0;
	return tf;
}

TruthFunction TruthFunction::False()
{
	TruthFunction tf;
	tf.Init();
	tf.Logic = 0x0000;
	tf.Buffer[0] = 'F';
	tf.Buffer[1] = 0;
	return tf;
}

TruthFunction TruthFunction::FromInt(int i)
{
	if (i == 0)
		return False();
	else if (i == 1)
		return True();

	yyerrorf("Cannot create truth function from integer value: %d", i);    
	return False();
}

TruthFunction TruthFunction::FromSignal(Signal *signal)
{
	TruthFunction tf;
	tf.Init();
	tf.AddArg(signal);
	tf.Logic = 0xAAAA;
	tf.Buffer[0] = '0';
	tf.Buffer[1] = 0;
	return tf;
}



// Logic Operations

TruthFunction TruthFunction::NOT_TF() const
{
	TruthFunction result = *this;

	// (~a)
	// a~
	if (Logic == 0x0000)
	{
		strcpy(result.Buffer, "T");
	}
	else if (Logic == 0xFFFF)
	{
		strcpy(result.Buffer, "F");
	}
	else
	{
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, "~");
	}

	result.Logic ^= 0xFFFF;

	return result;
}

TruthFunction TruthFunction::AND_TF(const TruthFunction &tf) const
{
	TruthFunction result = Merge(tf);

	// (a & b)
	// ba&
	if (Logic == 0x0000)
	{
		// 0 & x = 0
		strcpy(result.Buffer, "F");
	}
	else if (Logic == 0xFFFF)
	{
		// 1 & x = x
		strcpy(result.Buffer, Buffer);
	}
	else
	{
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, Buffer);
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, "&");
	}

	result.Logic &= Logic;

	return result;
}

TruthFunction TruthFunction::OR_TF(const TruthFunction &tf) const
{
	TruthFunction result = Merge(tf);

	// (a | b)
	// ba|
	if (Logic == 0x0000)
	{
		// 0 | x = x
		strcpy(result.Buffer, Buffer);
	}
	else if (Logic == 0xFFFF)
	{
		// 1 | x = 1
		strcpy(result.Buffer, "T");
	}
	else
	{
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, Buffer);
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, "|");
	}

	result.Logic &= Logic;

	return result;
}

TruthFunction TruthFunction::XOR_TF(const TruthFunction &tf) const
{
	TruthFunction result = Merge(tf);

	// (a ^ b)
	// ba^
	if (Logic == 0x0000)
	{
		// 0 ^ x = x
		strcpy(result.Buffer, Buffer);
	}
	else if (Logic == 0xFFFF)
	{
		// 1 ^ x = ~x
		strcpy(result.Buffer, Buffer);
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, "~");
	}
	else
	{
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, Buffer);
		strcatbuf(result.Buffer, MAX_TF_BUF_LEN, "^");
	}

	result.Logic ^= Logic;

	return result;
}

TruthFunction TruthFunction::EQ_TF(const TruthFunction &tf) const
{
	// same as XOR
	return XOR_TF(tf);
}

TruthFunction TruthFunction::NE_TF(const TruthFunction &tf) const
{
	// same as XNOR
	return XOR_TF(tf).NOT_TF();
}

TruthFunction TruthFunction::Ternary_TF(const TruthFunction &tf1, const TruthFunction &tf2) const
{
	// (this && tf1) || (!this && tf2)
	return (this->AND_TF(tf1)).OR_TF(this->NOT_TF().AND_TF(tf2));
}



// Swap functions

// instance method to modify logic and buffer
void TruthFunction::SwapLogic(int a, int b)
{
	if (a == b) return;

	// Swap Buffer
	char ca = '0' + a;
	char cb = '0' + b;

	char *c = Buffer;
	while (*c)
	{
		if (*c == ca)
			*c = cb;
		else if (*c == cb)
			*c = ca;
		c++;
	}

	// Swap Logic
	Logic = SwapLogic(Logic, a, b);
}

// static function to compute logic
int TruthFunction::SwapLogic(int logic, int a, int b)
{
	// nothing to swap
	if (a == b)
		return logic;

	// ensure a < b
	if (b < a)
	{
		int tmp = a;
		a = b;
		b = tmp;
	}

	if (a == 0)
	{
		if (b == 1)
			return Swap01(logic);
		else if (b == 2)
			return Swap02(logic);
		else if (b == 3)
			return Swap03(logic);
	}
	else if (a == 1)
	{
		if (b == 2)
			return Swap12(logic);
		else if (b == 3)
			return Swap13(logic);
	}

	// a == 2,  b == 3
	return Swap23(logic);
}

int TruthFunction::SwapBits(int logic, int a, int b)
{
	int a_mask = 1 << a;
	int b_mask = 1 << b;
	
	int a_tmp = logic & a_mask;
	int b_tmp = logic & b_mask;

	int shift = b - a;
	a_tmp <<= shift;
	b_tmp >>= shift;
	
	int result = (logic & ~a_mask & ~b_mask) | (a_tmp | b_tmp);
	return result;
}

int TruthFunction::Swap01(int logic)
{
	logic = SwapBits(logic, 1, 2);
	logic = SwapBits(logic, 5, 6);
	logic = SwapBits(logic, 9, 10);
	logic = SwapBits(logic, 13,14);
	return logic;
}

int TruthFunction::Swap02(int logic)
{
	logic = SwapBits(logic, 1, 4);
	logic = SwapBits(logic, 3, 6);
	logic = SwapBits(logic, 9, 12);
	logic = SwapBits(logic, 11,14);
	return logic;
}

int TruthFunction::Swap03(int logic)
{
	logic = SwapBits(logic, 1, 8);
	logic = SwapBits(logic, 3, 10);
	logic = SwapBits(logic, 5, 12);
	logic = SwapBits(logic, 7, 14);
	return logic;
}

int TruthFunction::Swap12(int logic)
{
	logic = SwapBits(logic, 2, 4);
	logic = SwapBits(logic, 3, 5);
	logic = SwapBits(logic, 10,12);
	logic = SwapBits(logic, 11,13);
	return logic;
}

int TruthFunction::Swap13(int logic)
{
	logic = SwapBits(logic, 2, 8);
	logic = SwapBits(logic, 3, 9);
	logic = SwapBits(logic, 6, 12);
	logic = SwapBits(logic, 7, 13);
	return logic;
}

int TruthFunction::Swap23(int logic)
{
	logic = SwapBits(logic, 4, 8);
	logic = SwapBits(logic, 5, 9);
	logic = SwapBits(logic, 6, 10);
	logic = SwapBits(logic, 7, 11);
	return logic;
}




TruthFunction TruthFunction::Merge(const TruthFunction &tf) const
{
	int i1;
	int i2;

	// Result contains Logic of tf argument, possibly modified, and merged Args array
	TruthFunction result;
	result.Logic = tf.Logic;
	strcpy(result.Buffer, tf.Buffer);

	// Start out with original Args array
	for (i1=0; i1 < 4; i1++)
	{
		result.Args[i1] = Args[i1];
	}

	// Copy each Arg from tf
	for (i2=0; i2 < 4; i2++)
	{
		Signal *sig2 = tf.Args[i2];

		if (sig2 == NULL)
			break;

		for (i1=0; i1 < 4; i1++)
		{
			Signal *sig1 = result.Args[i1];
			if (sig1 == NULL || sig1 == sig2)
				break;
		}

		if (i1 >= 4)
		{
			yyerror("Cannot create TF with more than 4 arguments");
			return result;
		}

		if (i1 != i2)
		{
			result.Args[i1] = sig2;
			result.SwapLogic(i2, i1);
		}
	}

	return result;
}


// Generate a string Verilog expression of the TF logic
const char *TruthFunction::ToVerilogExpression(char *buf, int buflen) const
{
	if (buflen > 1)
	{
		// Clear buffer
		buf[0] = 0;

		// Recursively generate expression
		bufferLocation = strlen(Buffer) - 1;            // static variable used throughout recursion
		if (WriteVerilogExpression(buf, buflen-1))
			return buf;
	}
	
	yyerrorf("Unable to generate Verilog expression for truth function longer than %d characters", buflen-1);
	return "UNKNOWN";
}

// Recursive call to generate verilog expression
bool TruthFunction::WriteVerilogExpression(char *buf, int buflen) const
{
	if (bufferLocation < 0)
		return false;

	char c = Buffer[bufferLocation];
	bufferLocation--;

	switch (c)
	{
		case 'F':   strcatbuf(buf, buflen, "1'b0");  break;
		case 'T':   strcatbuf(buf, buflen, "1'b1");  break;

		case '0':   strcatbuf(buf, buflen, Args[0]->Name());  break;
		case '1':   strcatbuf(buf, buflen, Args[1]->Name());  break;
		case '2':   strcatbuf(buf, buflen, Args[2]->Name());  break;
		case '3':   strcatbuf(buf, buflen, Args[3]->Name());  break;

		case '~':
			strcatbuf(buf, buflen, "~");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			break;
			
		case '|':
			strcatbuf(buf, buflen, "(");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, " | ");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, ")");
			break;
			
		case '&':
			strcatbuf(buf, buflen, "(");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, " & ");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, ")");
			break;

		case '^':
			strcatbuf(buf, buflen, "(");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, " ^ ");
			if (!WriteVerilogExpression(buf, buflen)) return false;
			strcatbuf(buf, buflen, ")");
			break;

		default:
			// Unrecognized character in Buffer
			return false;
	}

	return true;
}

// static variable used throughout recursion
int TruthFunction::bufferLocation = -1;

