
#include "StringBuffer.h"
#include <string.h>
#include <stdlib.h>

StringBuffer::StringBuffer(int initial_size)
{
	buffer_size = initial_size;
	buffers[0] = (char *) malloc(buffer_size);

	buffer = 0;
	buffers[0][0] = 0;
	num_strings = 0;
	next = buffers[0];
	start = buffers[0];
}

StringBuffer::~StringBuffer()
{
	for (int i=0; i <= buffer; i++)
		free(buffers[i]);
}

void StringBuffer::Reset()
{
	if (buffer > 0)
	{
		for (int i=0; i <= buffer; i++)
			free(buffers[i]);

		buffers[0] = (char *) malloc(buffer_size);
	}

	buffer = 0;
	buffers[0][0] = 0;
	num_strings = 0;
	next = buffers[0];
	start = buffers[0];
}

int StringBuffer::NumStrings() const
{
	return num_strings;
}

char *StringBuffer::AddString(const char *s)
{
	int len = strlen(s);
	EnsureBuffer(len + 1);

	strcpy(next, s);
	next += (len + 1);

	char *result = start;
	start = next;

	num_strings++;

	return result;
}

void StringBuffer::StartString()
{
	*next = 0;
	start = next;
}

void StringBuffer::AppendString(const char *s)
{
	int len = strlen(s);
	EnsureBuffer(len + 1);

	char *dest = next;
	strcpy(dest, s);
	next += len;
}

void StringBuffer::AppendChar(char c)
{
	EnsureBuffer(2);

	*next = c;
	next++;
}

char *StringBuffer::FinishString()
{
	*next = 0;
	next++;
	num_strings++;

	char *result = start;
	start = next;

	return result;
}

int StringBuffer::EnsureBuffer(int size)
{
	if (next + size <= buffers[buffer] + buffer_size)
		return buffer_size;

	if (buffer >= MAX_STRING_BUFFERS-1)
		return 0;

	int total_size = size + (next-start);
	do {
		buffer_size *= 2;
	}
	while (buffer_size < total_size);

	buffer++;
	char *newbuf = (char *) malloc(buffer_size);
	buffers[buffer] = newbuf;

	if (start != next)
	{
		*next = 0;
		strcpy(newbuf, start);
		next = newbuf + (next - start);
		start = newbuf;
	}
	else
	{
		next = newbuf;
		start = newbuf;
	}

	return buffer_size;
}
