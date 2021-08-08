
#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#define MAX_STRING_BUFFERS (64)

class StringBuffer
{
public:
	StringBuffer(int initial_size = 4096);
	virtual ~StringBuffer();

	void Reset();

	int NumStrings() const;

	char *AddString(const char *s);

	void StartString();
	void AppendString(const char *s);
	void AppendChar(char c);
	char *FinishString();

protected:
	int EnsureBuffer(int size);

private:
	char *buffers[MAX_STRING_BUFFERS];
	int buffer;
	int buffer_size;
	int num_strings;
	char *next;
	char *start;
};

#endif
