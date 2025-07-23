%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "alpha_lexer.h"
#include "syntax.tab.h"


extern YYSTYPE yylval; // external variable
extern int yylineno;   // Initialize yylineno
int comment_depth = 0; // Initialize comment depth for nested comments
int comment_start_line = 0; // Initialize starting line for multi-line comments

char *strdup(const char*);
char* process_escaped_characters(const char* input); // Declare the helper function

int yywrap(void) {
    return 1;
}


%}

%x COMMENT

%%

\"([^\\\"]|\\[ntbrfv0\"\\])* {
    fprintf(stderr, "Error: Unterminated string at line %d: '%s'.\n", yylineno, yytext);
    yylval.strval = strdup(yytext); // Capture the unterminated string
    BEGIN(INITIAL);                 // Ensure the lexer returns to the initial state
    return UNKNOWN;                 // Return an error token
}

\"([^\\\"]|\\[ntbrfv0\"\\])*\" {
    yylval.strval = strdup(yytext);

    // Remove the surrounding quotes
    yylval.strval[strlen(yylval.strval) - 1] = '\0';
    memmove(yylval.strval, yylval.strval + 1, strlen(yylval.strval));

    // Process escaped characters
    char *processed_str = process_escaped_characters(yylval.strval);
    free(yylval.strval);
    yylval.strval = processed_str;

    return STRINGCONST;
}

\"([^\\\"]|\\[ntbrfv0\"\\])*\\[^ntbrfv0\"\\]([^\\\"]|\\[ntbrfv0\"\\])*\" {
    fprintf(stderr, "Warning: Invalid escape character in string at line %d.\n", yylineno);
    yylval.strval = strdup(yytext);

    // Remove the surrounding quotes
    yylval.strval[strlen(yylval.strval) - 1] = '\0';
    memmove(yylval.strval, yylval.strval + 1, strlen(yylval.strval));

    // Process escaped characters
    char *processed_str = process_escaped_characters(yylval.strval);
    free(yylval.strval);
    yylval.strval = processed_str;

    return STRINGCONST;
}

"int" {
    yylval.strval = strdup(yytext);
    return KEYWORD_INT;
}

"double" {
    yylval.strval = strdup(yytext);
    return KEYWORD_REAL;
}

"if" {
    yylval.strval = strdup(yytext);
    return KEYWORD_IF;
}

"else" {
    yylval.strval = strdup(yytext);
    return KEYWORD_ELSE;
}

"while" {
    yylval.strval = strdup(yytext);
    return KEYWORD_WHILE;
}

"for" {
    yylval.strval = strdup(yytext);
    return KEYWORD_FOR;
}

"function" {
    yylval.intval = yylineno; // Store the current line number in yylval
    return KEYWORD_FUNCTION;
}

"return" {
    yylval.strval = strdup(yytext);
    return KEYWORD_RETURN;
}

"break" {
    yylval.strval = strdup(yytext);
    return KEYWORD_BREAK;
}

"continue" {
    yylval.strval = strdup(yytext);
    return KEYWORD_CONTINUE;
}

"and" {
    yylval.strval = strdup(yytext);
    return KEYWORD_AND;
}

"not" {
    yylval.strval = strdup(yytext);
    return KEYWORD_NOT;
}

"or" {
    yylval.strval = strdup(yytext);
    return KEYWORD_OR;
}

"local" {
    yylval.strval = strdup(yytext);
    return KEYWORD_LOCAL;
}

"true" {
    yylval.strval = strdup(yytext);
    return KEYWORD_TRUE;
}

"false" {
    yylval.strval = strdup(yytext);
    return KEYWORD_FALSE;
}

"nil" {
    yylval.strval = strdup(yytext);
    return KEYWORD_NIL;
}

[0-9]+\.[0-9]+ {
    yylval.doubleval = atof(yytext); // Store the real constant in doubleval
    return REALCONST;
}

[0-9]+ {
    yylval.intval = atoi(yytext); // Store the integer constant in intval
    return INTCONST;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.strval = strdup(yytext); // Store the identifier in strval
    return IDENT;
}

[ \t\r\n]+ {
    /* Ignore whitespace (spaces, tabs, newlines, etc.) */
}

"//".* {
    /* Ignore the content of the comment */
}

"/*" {
    BEGIN(COMMENT);
    comment_depth = 1; // Start tracking the depth of comments
    comment_start_line = yylineno; // Record the starting line of the comment
  
}

<COMMENT>{
    "/*" {
        comment_depth++; // Increase depth for nested comments
        
    }
    "*/" {
        comment_depth--; // Decrease depth when a comment closes
        if (comment_depth == 0) {
            BEGIN(INITIAL); // Return to the initial state
        } else {
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
    yylval.strval = strdup(yytext);
    return OPERATOR_EQUAL;
}

"+" {
    yylval.strval = strdup(yytext);
    return OPERATOR_PLUS;
}

"-" {
    yylval.strval = strdup(yytext);
    return OPERATOR_MINUS;
}

"*" {
    yylval.strval = strdup(yytext);
    return OPERATOR_MULTIPLY;
}

"/" {
    yylval.strval = strdup(yytext);
    return OPERATOR_DIVIDE;
}

"%" {
    yylval.strval = strdup(yytext);
    return OPERATOR_MODULO;
}

"==" {
    yylval.strval = strdup(yytext);
    return OPERATOR_EQUAL_EQUAL;
}

"!=" {
    yylval.strval = strdup(yytext);
    return OPERATOR_NOT_EQUAL;
}

"++" {
    yylval.strval = strdup(yytext);
    return OPERATOR_INCREMENT;
}

"--" {
    yylval.strval = strdup(yytext);
    return OPERATOR_DECREMENT;
}

">" {
    yylval.strval = strdup(yytext);
    return OPERATOR_GREATER;
}

"<" {
    yylval.strval = strdup(yytext);
    return OPERATOR_LESS;
}

">=" {
    yylval.strval = strdup(yytext);
    return OPERATOR_GREATER_EQUAL;
}

"<=" {
    yylval.strval = strdup(yytext);
    return OPERATOR_LESS_EQUAL;
}

"(" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_LEFT_PARENTHESIS;
}

")" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_RIGHT_PARENTHESIS;
}

"{" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_LEFT_BRACE;
}

"}" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_RIGHT_BRACE;
}

"[" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_LEFT_BRACKET;
}

"]" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_RIGHT_BRACKET;
}

";" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_SEMICOLON;
}

"," {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_COMMA;
}

":" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_COLON;
}

"::" {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_DOUBLE_COLON;
}

".." {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_DOUBLE_DOT;
}

"." {
    yylval.strval = strdup(yytext);
    return PUNCTUATION_DOT;
}



. {
    fprintf(stderr, "Error: Undefined input '%s' at line %d.\n", yytext, yylineno);
    yylval.strval = strdup(yytext); // Store the undefined input in strval
    return UNKNOWN; // Consume the input and move forward
}

%%



int alpha_yylex(void *ylval) {
    YYSTYPE *token = (YYSTYPE *)ylval; // Cast ylval to YYSTYPE
    int token_type = yylex(); // Call the lexer to get the next token

    if (token_type == 0) {
        return 0; // EOF
    }

    *token = yylval; // Assign the token value to the provided pointer

    // Print the token
    static int token_number = 1;
    print_token(yylineno, token_number++, token_type, yylval);

    return token_type; // Return the token type
}




void print_token(int line_number, int token_number, int token_type, YYSTYPE yylval) {
    printf("%d: #%d ", line_number, token_number);

    switch (token_type) {
        case IDENT:
            printf("\"%s\" IDENT\n", yylval.strval);
            break;
        case INTCONST:
            printf("\"%d\" INTCONST\n", yylval.intval);
            break;
        case REALCONST:
            printf("\"%f\" REALCONST\n", yylval.doubleval);
            break;
        case STRINGCONST:
            printf("\"%s\" STRINGCONST\n", yylval.strval);
            break;
        case KEYWORD_INT:
            printf("\"int\" KEYWORD_INT\n");
            break;
        case KEYWORD_REAL:
            printf("\"double\" KEYWORD_REAL\n");
            break;
        case OPERATOR_EQUAL:
            printf("\"=\" OPERATOR_EQUAL\n");
            break;
        case OPERATOR_PLUS:
            printf("\"+\" OPERATOR_PLUS\n");
            break;
        case OPERATOR_MINUS:
            printf("\"-\" OPERATOR_MINUS\n");
            break;
        case OPERATOR_MULTIPLY:
            printf("\"*\" OPERATOR_MULTIPLY\n");
            break;
        case OPERATOR_DIVIDE:
            printf("\"/\" OPERATOR_DIVIDE\n");
            break;
        case OPERATOR_MODULO:
            printf("\"%%\" OPERATOR_MODULO\n");
            break;
        case PUNCTUATION_SEMICOLON:
            printf("\";\" PUNCTUATION_SEMICOLON\n");
            break;
        case PUNCTUATION_LEFT_PARENTHESIS:
            printf("\"(\" PUNCTUATION_LEFT_PARENTHESIS\n");
            break;
        case PUNCTUATION_RIGHT_PARENTHESIS:
            printf("\")\" PUNCTUATION_RIGHT_PARENTHESIS\n");
            break;
        case PUNCTUATION_LEFT_BRACE:
            printf("\"{\" PUNCTUATION_LEFT_BRACE\n");
            break;
        case PUNCTUATION_RIGHT_BRACE:
            printf("\"}\" PUNCTUATION_RIGHT_BRACE\n");
            break;
        case PUNCTUATION_COMMA:
            printf("\",\" PUNCTUATION_COMMA\n");
            break;
        case UNKNOWN:
            printf("\"%s\" UNKNOWN\n", yylval.strval);
            break;
        default:
            printf("UNKNOWN TOKEN TYPE: %d\n", token_type);
            break;
    }
}

char* process_escaped_characters(const char* input) {
    char *output = malloc(strlen(input) + 1); // Allocate enough space for the processed string
    if (!output) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(1);
    }

    char *out_ptr = output;
    const char *in_ptr = input;

    while (*in_ptr) {
        if (*in_ptr == '\\') {
            in_ptr++;
            switch (*in_ptr) {
                case 'n': *out_ptr++ = '\n'; break; // Newline
                case 't': *out_ptr++ = '\t'; break; // Tab
                case 'b': *out_ptr++ = '\b'; break; // Backspace
                case 'r': *out_ptr++ = '\r'; break; // Carriage return
                case 'f': *out_ptr++ = '\f'; break; // Form feed
                case 'v': *out_ptr++ = '\v'; break; // Vertical tab
                case '0': *out_ptr++ = '\0'; break; // Null character
                case '\\': *out_ptr++ = '\\'; break; // Backslash
                case '\"': *out_ptr++ = '\"'; break; // Double quote
                default:
                    fprintf(stderr, "Warning: Invalid escape character '\\%c' in string.\n", *in_ptr);
                    *out_ptr++ = '\\'; // Keep the backslash
                    *out_ptr++ = *in_ptr; // Keep the invalid character
                    break;
            }
        } else {
            *out_ptr++ = *in_ptr; // Copy normal characters
        }
        in_ptr++;
    }
    *out_ptr = '\0'; // Null-terminate the processed string
    return output;
}
/*
int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            perror("Error opening input file");
            return 1;
        }
    }

    int token_type;
    static int token_number = 1;

    // Call the lexer directly and print tokens
    while ((token_type = yylex()) != 0) {
        print_token(yylineno, token_number++, token_type, yylval);
    }

    if (yyin) {
        fclose(yyin);
    }

    return 0;
}
*/