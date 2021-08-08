
#include <stdio.h>
#include "Signal.h"
#include "TruthFunction.h"


void TestTF(char *msg, const TruthFunction &tf)
{
	printf("\n%s\n", msg);
	tf.Print(stdout);
	printf("\n");

	char buf[16];
	printf("%s\n", tf.ToVerilogExpression(buf, sizeof(buf)));
}


int main(int argc, char *argv[])
{
	Signal *a = new Signal("a", BEHAVIOR_WIRE, DATA_TYPE_BIT, DIR_NONE, 0);
	Signal *b = new Signal("b", BEHAVIOR_WIRE, DATA_TYPE_BIT, DIR_NONE, 1);
	Signal *c = new Signal("c", BEHAVIOR_WIRE, DATA_TYPE_BIT, DIR_NONE, 1);
	Signal *d = new Signal("d", BEHAVIOR_WIRE, DATA_TYPE_BIT, DIR_NONE, 0);
	Signal *e = new Signal("e", BEHAVIOR_WIRE, DATA_TYPE_BIT, DIR_NONE, 0);

	TruthFunction tf_int0 = TruthFunction::FromInt(0);
	tf_int0.Print(stdout);
	printf("\n");


	TruthFunction tf_a = TruthFunction::FromSignal(a);
	TruthFunction tf_b = TruthFunction::FromSignal(b);
	TruthFunction tf_c = TruthFunction::FromSignal(c);
	TruthFunction tf_d = TruthFunction::FromSignal(d);
	TruthFunction tf_e = TruthFunction::FromSignal(e);
	tf_a.Print(stdout);
	printf("\n");

/*
	int swapbits   = 0x1248;
	int swapbits03 = TruthFunction::SwapBits(swapbits, 0, 3);
	printf("SwapBits03: 0x%04X -> 0x%04X\n", swapbits, swapbits03);

	int swap2  = 0xABCD;
	int swap03 = TruthFunction::Swap03(swap2);
	printf("Swap03: 0x%04X -> 0x%04X\n", swap2, swap03);


	printf("\nMerge a + b:\n");
	tf_a.Print(stdout);
	tf_b.Print(stdout);
	TruthFunction tf_ab = tf_a.Merge(tf_b);
	printf("Result ab:\n");
	tf_ab.Print(stdout);

	printf("\nMerge ab + d:\n");
	tf_ab.Print(stdout);
	tf_d.Print(stdout);
	printf("Result abd:\n");
	TruthFunction tf_abd = tf_ab.Merge(tf_d);
	tf_abd.Print(stdout);
*/

	TruthFunction aAb           = tf_a.AND_TF(tf_b.NOT_TF());
	TruthFunction aAbOd         = aAb.OR_TF(tf_d);
	TruthFunction aAbOdAb       = aAbOd.AND_TF(tf_b);
	TruthFunction dAb           = tf_d.AND_TF(tf_b);

	TruthFunction dTb_c         = tf_d.Ternary_TF(tf_b, tf_c);
	TruthFunction dAb_O_NdAc    = (tf_d.AND_TF(tf_b)).OR_TF(tf_d.NOT_TF().AND_TF(tf_c));

	TestTF("a && !b",                   aAb );
	TestTF("(a && !b) || d",            aAbOd );
	TestTF("((a && !b) || d) && b",     aAbOdAb );
	TestTF("d && b",                    dAb );
	TestTF("d ? b : c",                 dTb_c );
	TestTF("(d && b) || (~d && c)",     dAb_O_NdAc );

	return 0;
}


