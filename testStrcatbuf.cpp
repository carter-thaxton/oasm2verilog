
#include <stdio.h>
#include <string.h>

// static helper function to concatenate onto a string in a fixed-size buffer
// writes as much as possible into the buffer, and always null-terminates the result
// returns the length of the final result, without the null-terminator
static int strcatbuf(char *buf, int bufsize, const char *str)
{
	int buflen = strlen(buf);
	int len = strlen(str);

	int bufleft = bufsize - buflen - 1;
	if (bufleft <= 0)
		return bufsize-1;
	
	int catlen = (len < bufleft) ? len : bufleft;
	int newlen = buflen + catlen;

	printf("bufsize: %d   buflen: %d   len: %d   bufleft: %d   catlen: %d  newlen: %d\n",
		bufsize, buflen, len, bufleft, catlen, newlen);

	strncpy(&buf[buflen], str, catlen);
	buf[newlen] = 0;

	return newlen;
}

int main()
{
	char buf[16];
	strcpy(buf, "abcdefg");

	int newlen = strcatbuf(buf, sizeof(buf), "123456789");

	printf("result: %s  (%d)\n", buf, newlen);

	return 0;
}
