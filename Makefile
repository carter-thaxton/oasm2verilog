
# Tools
CXX = g++
CXXFLAGS = $(NO_CYGWIN) -Wall -g $(PROFILE) $(DEFINES)
LEX	= flex
LFLAGS	=
YACC	= bison
YFLAGS	= -v --debug

# Options for profiling and debugging
#PROFILE = -pg
#DEFINES = -DDEBUG
#NO_CYGWIN = -mno-cygwin

# Files
TARGET	= oasm2verilog
OBJ	= parser.o oasm2verilog.o lex.o parse.tab.o Common.o IllegalNames.o \
	  StringBuffer.o StringMap.o SymbolTable.o Symbol.o Identifier.o \
	  Signal.o Module.o Instance.o Connection.o SiliconObject.o SiliconObjectRegistry.o \
	  Expression.o Variable.o EnumValue.o Parameter.o \
	  Function.o BuiltinFunction.o TruthFunction.o \
	  AluFunction.o AluInstruction.o Alu.o \
	  FPOA.o TF.o RF.o \
	  Module_GenerateVerilog.o SiliconObject_GenerateVerilog.o FPOA_GenerateVerilog.o Alu_GenerateVerilog.o TF_GenerateVerilog.o

LIB	= -lm

# Rules
$(TARGET):	$(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(LIB) $(OBJ)

parse.tab.cpp:	parse.y
	$(YACC) $(YFLAGS) -o $@ $<

lex.cpp:	lex.l
	$(LEX) $(LFLAGS) -t $< > $@

.SECONDARY: lex.cpp parse.tab.cpp


# Dependencies
parser.o:		parse.tab.cpp parser.h

# Cleanup
clean:
	rm -f $(TARGET) $(TARGET).exe *.exe $(OBJ) *.o *.stackdump *.bak lex.yy.c *.tab.cpp *.tab.hpp *.output *.v lex.c gmon.out gmon.txt


# Tests
TF_OBJ	= testTruthFunction.o testCommon.o StringBuffer.o Symbol.o TruthFunction.o Signal.o Expression.o
testTruthFunction:	$(TF_OBJ)
	$(CXX) $(CXXFLAGS) -o testTruthFunction $(LIB) $(TF_OBJ)

