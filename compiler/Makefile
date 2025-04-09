# config.mk contains the paths to antlr4 etc.
# Each student should have a config.mk corresponding to her system.
# Examples are  ubuntu.mk, DI.mk, fedora.mk
# Then config.mk should be in the .gitignore of your project
include config.mk

CC = g++
CCFLAGS = -g -c -std=c++17 -I$(ANTLRINC) -Wno-attributes
LDFLAGS = -g

default: all
all: ifcc

##########################################
# link together all pieces of our compiler 
OBJECTS = build/ifccBaseVisitor.o \
          build/ifccLexer.o \
          build/ifccVisitor.o \
          build/ifccParser.o \
          build/main.o \
          build/CodeGenVisitor.o \
          build/SymbolTable.o \
          build/IR.o \
		  build/gen_asm_arm.o \
		  build/gen_asm_msp430.o \
		  build/gen_asm_x86.o \
		  build/gen_asm_arm64.o \

ifcc: $(OBJECTS)
	@mkdir -p build
	$(CC) $(LDFLAGS) build/*.o $(ANTLRLIB) -o ifcc

##########################################
# compile our hand-written C++ code: main(), CodeGenVisitor, SymbolTable, IR, etc.
build/%.o: %.cpp generated/ifccParser.cpp
	@mkdir -p build
	$(CC) $(CCFLAGS) -MMD -o $@ $< 

##########################################
# compile all the antlr-generated C++ sources
build/%.o: generated/%.cpp
	@mkdir -p build
	$(CC) $(CCFLAGS) -MMD -o $@ $< 

# automagic dependency management: `gcc -MMD` generates all the .d files for us
-include build/*.d
build/%.d:

##########################################
# generate the C++ implementation of our Lexer/Parser/Visitor from the grammar ifcc.g4
generated/ifccLexer.cpp: generated/ifccParser.cpp
generated/ifccVisitor.cpp: generated/ifccParser.cpp
generated/ifccBaseVisitor.cpp: generated/ifccParser.cpp
generated/ifccParser.cpp: ifcc.g4
	@mkdir -p generated
	$(ANTLR) -visitor -no-listener -Dlanguage=Cpp -o generated ifcc.g4

# prevent automatic cleanup of "intermediate" files like ifccLexer.cpp etc.
.PRECIOUS: generated/ifcc%.cpp   

##########################################
# view the parse tree in a graphical window
# Usage: `make gui FILE=path/to/your/file.c`
FILE ?= ../tests/testfiles/1_return42.c

gui:
	@mkdir -p generated build
	$(ANTLR) -Dlanguage=Java -o generated ifcc.g4
	javac -cp $(ANTLRJAR) -d build generated/*.java
	java -cp $(ANTLRJAR):build org.antlr.v4.gui.TestRig ifcc axiom -gui $(FILE)

##########################################
# delete all machine-generated files
clean:
	rm -rf build generated
	rm -f ifcc

##########################################
# run the gcc and ifcc compiler
run: ifcc
	./ifcc ret42.c > ret42_ifcc.s
	gcc -S ret42.c -o ret42_gcc.s
	gcc ret42_ifcc.s
	./a.out
