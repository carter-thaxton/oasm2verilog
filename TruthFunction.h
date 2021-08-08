
#ifndef TRUTH_FUNCTION_H
#define TRUTH_FUNCTION_H

#include <stdio.h>

class Signal;

#define TF_ARG0		0xAAAA
#define TF_ARG1		0xCCCC
#define TF_ARG2		0xF0F0
#define TF_ARG3		0xFF00

#define MAX_TF_BUF_LEN      (64)

const int TF_ARG[4] = {TF_ARG0, TF_ARG1, TF_ARG2, TF_ARG3};

struct TruthFunction
{
	Signal *Args[4];
	int Logic;
	char Buffer[MAX_TF_BUF_LEN];

	void Init();
	int NumArgs() const;

	bool IsFalse() const;
	bool IsTrue() const;
	bool IsHold(const Signal *signal) const;

	void Print(FILE *f) const;

	// Factories for specific truth-functions
	static TruthFunction True();
	static TruthFunction False();
	static TruthFunction FromInt(int i);
	static TruthFunction FromSignal(Signal *signal);

	// Operations on TFs
	TruthFunction NOT_TF() const;
	TruthFunction AND_TF(const TruthFunction &tf) const;
	TruthFunction OR_TF(const TruthFunction &tf) const;
	TruthFunction XOR_TF(const TruthFunction &tf) const;
	TruthFunction EQ_TF(const TruthFunction &tf) const;
	TruthFunction NE_TF(const TruthFunction &tf) const;
	TruthFunction Ternary_TF(const TruthFunction &tf1, const TruthFunction &tf2) const;

	// Generate a string Verilog expression of the TF logic
	const char *ToVerilogExpression(char *buf, int buflen) const;

private:
	int AddArg(Signal *signal);		// Returns the new TF index, or -1 if none available
	TruthFunction Merge(const TruthFunction &tf) const;
	void SwapLogic(int a, int b);

	bool WriteVerilogExpression(char *buf, int buflen) const;
	static int bufferLocation;      // used in recursion

	static int SwapLogic(int logic, int a, int b);
	static int SwapBits(int logic, int a, int b);
	static int Swap01(int logic);
	static int Swap02(int logic);
	static int Swap03(int logic);
	static int Swap12(int logic);
	static int Swap13(int logic);
	static int Swap23(int logic);
};

#endif
