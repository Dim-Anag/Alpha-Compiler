
# Compiler and flags
CC = gcc		
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -Wno-unused-function -Wno-unused-label -Wno-implicit-function-declaration -D__USE_MINGW_ANSI_STDIO

# Flex and Bison
FLEX = flex		
FLEX_FLAGS =		

# Source files
LEXER_SRC = main.l		
LEXER_GEN = lex.yy.c		
PARSER_SRC = al.c		
HEADERS = alpha_lexer.h		

# Executable
EXEC = lexer		

# Default target
all: $(EXEC)		

# Compile the lexer
$(LEXER_GEN): $(LEXER_SRC)		
	$(FLEX) $(FLEX_FLAGS) $(LEXER_SRC)		

# Compile the executable
$(EXEC): $(LEXER_GEN) $(PARSER_SRC) $(HEADERS)		
	$(CC) $(CFLAGS) $(LEXER_GEN) $(PARSER_SRC) -o $(EXEC) 		

# Clean up generated files
clean:
	rm -f $(LEXER_GEN) $(EXEC)		

# Phony targets
.PHONY:	all clean			