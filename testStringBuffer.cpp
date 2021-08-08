
#include <stdio.h>
#include "StringBuffer.h"

int main(int argc, char *argv[])
{
	StringBuffer strings_local(4);
	StringBuffer *strings = &strings_local;

	for (int i=0; i < 4; i++)
	{
		strings->Reset();
		printf("\n\n== ITERATION %d ==\n\n", i);

		char *hey = strings->AddString("hey\n");
		printf("%s", hey);

		strings->StartString();
		strings->AppendString("start");
		strings->AppendChar('A');
		strings->AppendChar('B');
		strings->AppendChar('C');
		strings->AppendString("end\n");
		char *str = strings->FinishString();
		printf("%s", str);

		strings->StartString();
		strings->AppendString("start");
		strings->AppendChar('A');
		strings->AppendChar('B');
		strings->AppendChar('C');
		strings->AppendString("end\n");
		char *str2 = strings->FinishString();
		printf("%s", str2);

		char *there = strings->AddString("there\n");
		printf("%s", there);

		char *there2 = strings->AddString("there\n");
		printf("%s", there2);

		strings->StartString();
		strings->AppendString("start");
		strings->AppendChar('A');
		strings->AppendChar('B');
		strings->AppendChar('C');
		strings->AppendString("XXXX_more_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuff");
		strings->AppendString("YYYY_more_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuff");
		strings->AppendString("ZZZZ_more_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuffmore_Really_Long_Stuff");
		strings->AppendString("end\n");
		char *str3 = strings->FinishString();
		printf("%s", str3);
	}

	printf("\n\n== DONE ==\n\n");

	return 0;
}

