#ifndef ALPHA_LEXER_H
#define ALPHA_LEXER_H

#include "syntax.tab.h"
/*
typedef enum {
    
    // General categories
    KEYWORD,
    OPERATOR,
    PUNCTUATION,
    IDENT,
    INTCONST,
    REALCONST,
    STRINGCONST,
    UNKNOWN,

    // Specific subcategories for keywords
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_FUNCTION,
    KEYWORD_RETURN,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_AND,
    KEYWORD_NOT,
    KEYWORD_OR,
    KEYWORD_LOCAL,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_NIL,

    // Specific subcategories for operators
    OPERATOR_EQUAL,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_MODULO,
    OPERATOR_EQUAL_EQUAL,
    OPERATOR_NOT_EQUAL,
    OPERATOR_INCREMENT,
    OPERATOR_DECREMENT,
    OPERATOR_GREATER,
    OPERATOR_LESS,
    OPERATOR_GREATER_EQUAL,
    OPERATOR_LESS_EQUAL,

    // Specific subcategories for punctuation
    PUNCTUATION_LEFT_PARENTHESIS,
    PUNCTUATION_RIGHT_PARENTHESIS,
    PUNCTUATION_LEFT_BRACE,
    PUNCTUATION_RIGHT_BRACE,
    PUNCTUATION_LEFT_BRACKET,
    PUNCTUATION_RIGHT_BRACKET,
    PUNCTUATION_SEMICOLON,
    PUNCTUATION_COMMA,
    PUNCTUATION_COLON,
    PUNCTUATION_DOUBLE_COLON,
    PUNCTUATION_DOT,
    PUNCTUATION_DOUBLE_DOT
    
} alpha_token_category_t;

typedef struct {
    alpha_token_category_t category;     // General category
    alpha_token_category_t subcategory; // Specific subcategory
    union {
        int int_val;
        char *str_val;
        double real_val;
    } attribute;
} alpha_token_t;
*/
int alpha_yylex(void *ylval);
void print_token(int line_number, int token_number, int token_type, YYSTYPE yylval);
#endif // ALPHA_LEXER_H