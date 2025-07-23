%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "alpha_lexer.h"

#define YYSTYPE alpha_token_t

extern YYSTYPE yylval; // external variable
extern int yylineno;   // Initialize yylineno
int comment_depth = 0; // Initialize comment depth for nested comments
int comment_start_line = 0; // Initialize starting line for multi-line comments

char *strdup(const char*);
char* process_escaped_characters(const char* input); // Declare the helper function
%}

%x COMMENT

%%

\"([^\\\"]|\\[ntbrfv0\"\\])* { 
    fprintf(stderr, "Error: Unterminated string at line %d.\n", yylineno);
    yylval.category = UNKNOWN;
    yylval.subcategory = UNKNOWN;
    yylval.attribute.str_val = strdup(yytext); // Capture the unterminated string
    return UNKNOWN; // Consume the input and move forward
}

\"([^\\\"]|\\[ntbrfv0\"\\])*\" {
    yylval.category = STRINGCONST;
    yylval.subcategory = STRINGCONST;
    yylval.attribute.str_val = strdup(yytext);

    // Remove the surrounding quotes
    yylval.attribute.str_val[strlen(yylval.attribute.str_val) - 1] = '\0';
    memmove(yylval.attribute.str_val, yylval.attribute.str_val + 1, strlen(yylval.attribute.str_val));

    // Process escaped characters
    char *processed_str = process_escaped_characters(yylval.attribute.str_val);
    free(yylval.attribute.str_val);
    yylval.attribute.str_val = processed_str;

    return STRINGCONST;
}

\"([^\\\"]|\\[ntbrfv0\"\\])*\\[^ntbrfv0\"\\]([^\\\"]|\\[ntbrfv0\"\\])*\" {
    fprintf(stderr, "Warning: Invalid escape character in string at line %d.\n", yylineno);
    yylval.category = STRINGCONST;
    yylval.subcategory = STRINGCONST;
    yylval.attribute.str_val = strdup(yytext);

    // Remove the surrounding quotes
    yylval.attribute.str_val[strlen(yylval.attribute.str_val) - 1] = '\0';
    memmove(yylval.attribute.str_val, yylval.attribute.str_val + 1, strlen(yylval.attribute.str_val));

    // Process escaped characters
    char *processed_str = process_escaped_characters(yylval.attribute.str_val);
    free(yylval.attribute.str_val);
    yylval.attribute.str_val = processed_str;

    return STRINGCONST;
}

"if" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_IF;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_IF;
}

"else" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_ELSE;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_ELSE;
}

"while" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_WHILE;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_WHILE;
}

"for" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_FOR;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_FOR;
}

"function" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_FUNCTION;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_FUNCTION;
}

"return" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_RETURN;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_RETURN;
}

"break" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_BREAK;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_BREAK;
}

"continue" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_CONTINUE;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_CONTINUE;
}

"and" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_AND;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_AND;
}

"not" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_NOT;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_NOT;
}

"or" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_OR;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_OR;
}

"local" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_LOCAL;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_LOCAL;
}

"true" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_TRUE;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_TRUE;
}

"false" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_FALSE;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_FALSE;
}

"nil" {
    yylval.category = KEYWORD;
    yylval.subcategory = KEYWORD_NIL;
    yylval.attribute.str_val = strdup(yytext);
    return KEYWORD_NIL;
}

[0-9]+\.[0-9]+ {
    yylval.category = REALCONST;
    yylval.subcategory = REALCONST; // No subcategory for REALCONST
    yylval.attribute.real_val = atof(yytext);
    return REALCONST;
}

[0-9]+ {
    yylval.category = INTCONST;
    yylval.subcategory = INTCONST; // No subcategory for INTCONST
    yylval.attribute.int_val = atoi(yytext);
    return INTCONST;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.category = IDENT;
    yylval.subcategory = IDENT; // No subcategory for IDENT
    yylval.attribute.str_val = strdup(yytext);
    return IDENT;
}

[ \t\r\n]+ {
    /* Ignore whitespace (spaces, tabs, newlines, etc.) */
}

"//".* {
    printf("Single-line comment at line %d\n", yylineno);
    /* Ignore the content of the comment */
}

"/*" {
    BEGIN(COMMENT);
    comment_depth = 1; // Start tracking the depth of comments
    comment_start_line = yylineno; // Record the starting line of the comment
    printf("Multi-line comment starts at line %d\n", comment_start_line);
}

<COMMENT>{
    "/*" {
        comment_depth++; // Increase depth for nested comments
        printf("Nested comment starts at line %d\n", yylineno);
    }
    "*/" {
        comment_depth--; // Decrease depth when a comment closes
        if (comment_depth == 0) {
            printf("Multi-line comment ends at line %d (from line %d)\n", yylineno, comment_start_line);
            BEGIN(INITIAL); // Return to the initial state
        } else {
            printf("Nested comment ends at line %d\n", yylineno);
        }
    }
    .|\n {
        /* Ignore all characters inside comments */
    }
}

<COMMENT><<EOF>> {
    fprintf(stderr, "Error: Unterminated comment starting at line %d.\n", comment_start_line);
    exit(1); // Exit if a comment is not closed before EOF
}




"=" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_EQUAL;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_EQUAL;
}

"+" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_PLUS;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_PLUS;
}

"-" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_MINUS;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_MINUS;
}

"*" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_MULTIPLY;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_MULTIPLY;
}

"/" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_DIVIDE;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_DIVIDE;
}

"%" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_MODULO;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_MODULO;
}

"==" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_EQUAL_EQUAL;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_EQUAL_EQUAL;
}

"!=" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_NOT_EQUAL;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_NOT_EQUAL;
}

"++" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_INCREMENT;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_INCREMENT;
}

"--" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_DECREMENT;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_DECREMENT;
}

">" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_GREATER;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_GREATER;
}

"<" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_LESS;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_LESS;
}

">=" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_GREATER_EQUAL;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_GREATER_EQUAL;
}

"<=" {
    yylval.category = OPERATOR;
    yylval.subcategory = OPERATOR_LESS_EQUAL;
    yylval.attribute.str_val = strdup(yytext);
    return OPERATOR_LESS_EQUAL;
}

"(" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_LEFT_PARENTHESIS;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_LEFT_PARENTHESIS;
}

")" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_RIGHT_PARENTHESIS;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_RIGHT_PARENTHESIS;
}

"{" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_LEFT_BRACE;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_LEFT_BRACE;
}

"}" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_RIGHT_BRACE;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_RIGHT_BRACE;
}

"[" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_LEFT_BRACKET;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_LEFT_BRACKET;
}

"]" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_RIGHT_BRACKET;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_RIGHT_BRACKET;
}

";" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_SEMICOLON;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_SEMICOLON;
}

"," {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_COMMA;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_COMMA;
}

":" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_COLON;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_COLON;
}

"::" {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_DOUBLE_COLON;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_DOUBLE_COLON;
}

"." {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_DOT;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_DOT;
}

".." {
    yylval.category = PUNCTUATION;
    yylval.subcategory = PUNCTUATION_DOUBLE_DOT;
    yylval.attribute.str_val = strdup(yytext);
    return PUNCTUATION_DOUBLE_DOT;
}

. {
    fprintf(stderr, "Error: Undefined input '%s' at line %d.\n", yytext, yylineno);
    yylval.category = UNKNOWN;
    yylval.subcategory = UNKNOWN;
    yylval.attribute.str_val = strdup(yytext);
    return UNKNOWN; // Consume the input and move forward
}

%%

int alpha_yylex(void *ylval) {
    alpha_token_t *token = (alpha_token_t *)ylval;
    int token_type = yylex();
    
    if (token_type == 0) {
        return 0; // EOF
    }
    *token = yylval;
    
    return token_type;
}