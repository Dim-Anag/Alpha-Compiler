# Makefile for building the parser

# Compiler and tools
FLEX = flex
BISON = bison
CC = gcc

# Files
LEX_FILE = main.l
YACC_FILE = syntax.y
SYMBOL_TABLE_FILE = symboltable.c
OUTPUT = parser

# Generated files
LEX_OUTPUT = lex.yy.c
YACC_OUTPUT = syntax.tab.c
YACC_HEADER = syntax.tab.h

# Build rules
all: $(OUTPUT)

$(OUTPUT): $(YACC_OUTPUT) $(LEX_OUTPUT) $(SYMBOL_TABLE_FILE)
	$(CC) -o $(OUTPUT) $(YACC_OUTPUT) $(LEX_OUTPUT) $(SYMBOL_TABLE_FILE)

$(YACC_OUTPUT): $(YACC_FILE)
	$(BISON) -d -v $(YACC_FILE)

$(LEX_OUTPUT): $(LEX_FILE)
	$(FLEX) $(LEX_FILE)

clean:
	rm -f $(OUTPUT) $(LEX_OUTPUT) $(YACC_OUTPUT) $(YACC_HEADER) syntax.output

.PHONY: all clean