# Makefile for building the parser

# Compiler and tools
FLEX = flex
BISON = bison
CC = gcc


# Files
LEX_FILE = main.l
YACC_FILE = syntax.y
SYMBOL_TABLE_FILE = symboltable.c
QUADS_FILE = quads.c
EXPR_FILE = expr.c
TEMP_FILE = temp.c
SCOPESPACE_FILE = scopespace.c
FUNCUTILS_FILE = funcutils.c
VARTEMP_FILE = vartempmap.c
AVM_CODEGEN_FILE = avm_codegen.c
AVM_CONST_TABLES_FILE = avm_const_tables.c
OUTPUT = parser 

# VM source files
VM_SRCS = avm.c avm_utils.c avm_tables.c avm_loader.c avm_libfuncs.c avm_execute.c avm_const_tables.c 
VM_HDRS = avm.h avm_utils.h avm_tables.h avm_libfuncs.h avm_instruction.h avm_const_tables.h 


# Generated files
LEX_OUTPUT = lex.yy.c
YACC_OUTPUT = syntax.tab.c
YACC_HEADER = syntax.tab.h
VM_OUTPUT = vm.exe


# Build rules
all: $(OUTPUT) $(VM_OUTPUT)

$(OUTPUT): $(YACC_OUTPUT) $(LEX_OUTPUT) $(SYMBOL_TABLE_FILE) $(QUADS_FILE) $(EXPR_FILE) $(TEMP_FILE) $(SCOPESPACE_FILE) $(FUNCUTILS_FILE) $(VARTEMP_FILE) $(AVM_CODEGEN_FILE) $(AVM_CONST_TABLES_FILE)
	$(CC) -o $(OUTPUT) $(YACC_OUTPUT) $(LEX_OUTPUT) $(SYMBOL_TABLE_FILE) $(QUADS_FILE) $(EXPR_FILE) $(TEMP_FILE) $(SCOPESPACE_FILE) $(FUNCUTILS_FILE) $(VARTEMP_FILE) $(AVM_CODEGEN_FILE) $(AVM_CONST_TABLES_FILE)


$(YACC_OUTPUT): $(YACC_FILE)
	$(BISON) -d -v $(YACC_FILE)

$(LEX_OUTPUT): $(LEX_FILE)
	$(FLEX) $(LEX_FILE)

$(VM_OUTPUT): $(VM_SRCS) $(VM_HDRS)
	$(CC) -o $(VM_OUTPUT) $(VM_SRCS) -lm

clean:
	rm -f $(OUTPUT) $(VM_OUTPUT) $(LEX_OUTPUT) $(YACC_OUTPUT) $(YACC_HEADER) syntax.output program.abc program.txt

.PHONY: all clean