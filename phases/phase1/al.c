#include <stdio.h>
#include <stdlib.h>
#include "alpha_lexer.h"
#include <string.h>

extern FILE *yyin;  // Declare yyin
extern int yylineno; // Declare yylineno

#define YYSTYPE alpha_token_t
YYSTYPE yylval; // Define yylval

int yywrap() {
    return 1;
}

void print_token(int line_number, int token_number, alpha_token_t *token) {
    printf("%d: #%d ", line_number, token_number);

    // Print the token content
    switch (token->category) {
        case INTCONST:
            printf("\"%d\" INTCONST\n", token->attribute.int_val);
            break;
        case REALCONST:
            printf("\"%f\" REALCONST\n", token->attribute.real_val);
            break;
        case STRINGCONST:
            printf("\"%s\" STRINGCONST\n", token->attribute.str_val);
            break;
        case IDENT:
            printf("\"%s\" IDENT\n", token->attribute.str_val);
            break;
        case UNKNOWN:
            printf("\"%s\" UNKNOWN\n", token->attribute.str_val);
            break;
        case KEYWORD:
            printf("\"%s\" KEYWORD ", token->attribute.str_val);
            switch (token->subcategory) {
                case KEYWORD_IF: printf("IF\n"); break;
                case KEYWORD_ELSE: printf("ELSE\n"); break;
                case KEYWORD_WHILE: printf("WHILE\n"); break;
                case KEYWORD_FOR: printf("FOR\n"); break;
                case KEYWORD_FUNCTION: printf("FUNCTION\n"); break;
                case KEYWORD_RETURN: printf("RETURN\n"); break;
                case KEYWORD_BREAK: printf("BREAK\n"); break;
                case KEYWORD_CONTINUE: printf("CONTINUE\n"); break;
                case KEYWORD_AND: printf("AND\n"); break;
                case KEYWORD_NOT: printf("NOT\n"); break;
                case KEYWORD_OR: printf("OR\n"); break;
                case KEYWORD_LOCAL: printf("LOCAL\n"); break;
                case KEYWORD_TRUE: printf("TRUE\n"); break;
                case KEYWORD_FALSE: printf("FALSE\n"); break;
                case KEYWORD_NIL: printf("NIL\n"); break;
                default: printf("UNKNOWN\n"); break;
            }
            break;
        case OPERATOR:
            printf("\"%s\" OPERATOR ", token->attribute.str_val);
            switch (token->subcategory) {
                case OPERATOR_EQUAL: printf("EQUAL\n"); break;
                case OPERATOR_PLUS: printf("PLUS\n"); break;
                case OPERATOR_MINUS: printf("MINUS\n"); break;
                case OPERATOR_MULTIPLY: printf("MULTIPLY\n"); break;
                case OPERATOR_DIVIDE: printf("DIVIDE\n"); break;
                case OPERATOR_MODULO: printf("MODULO\n"); break;
                case OPERATOR_EQUAL_EQUAL: printf("EQUAL_EQUAL\n"); break;
                case OPERATOR_NOT_EQUAL: printf("NOT_EQUAL\n"); break;
                case OPERATOR_INCREMENT: printf("INCREMENT\n"); break;
                case OPERATOR_DECREMENT: printf("DECREMENT\n"); break;
                case OPERATOR_GREATER: printf("GREATER\n"); break;
                case OPERATOR_LESS: printf("LESS\n"); break;
                case OPERATOR_GREATER_EQUAL: printf("GREATER_EQUAL\n"); break;
                case OPERATOR_LESS_EQUAL: printf("LESS_EQUAL\n"); break;
                default: printf("UNKNOWN\n"); break;
            }
            break;
        case PUNCTUATION:
            printf("\"%s\" PUNCTUATION ", token->attribute.str_val);
            switch (token->subcategory) {
                case PUNCTUATION_LEFT_PARENTHESIS: printf("LEFT_PARENTHESIS\n"); break;
                case PUNCTUATION_RIGHT_PARENTHESIS: printf("RIGHT_PARENTHESIS\n"); break;
                case PUNCTUATION_LEFT_BRACE: printf("LEFT_BRACE\n"); break;
                case PUNCTUATION_RIGHT_BRACE: printf("RIGHT_BRACE\n"); break;
                case PUNCTUATION_LEFT_BRACKET: printf("LEFT_BRACKET\n"); break;
                case PUNCTUATION_RIGHT_BRACKET: printf("RIGHT_BRACKET\n"); break;
                case PUNCTUATION_SEMICOLON: printf("SEMICOLON\n"); break;
                case PUNCTUATION_COMMA: printf("COMMA\n"); break;
                case PUNCTUATION_COLON: printf("COLON\n"); break;
                case PUNCTUATION_DOUBLE_COLON: printf("DOUBLE_COLON\n"); break;
                case PUNCTUATION_DOT: printf("DOT\n"); break;
                case PUNCTUATION_DOUBLE_DOT: printf("DOUBLE_DOT\n"); break;
                default: printf("UNKNOWN\n"); break;
            }
            break;
        default:
            printf("\"%s\" UNKNOWN\n", token->attribute.str_val);
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

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input file> [output file]\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (!input_file) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output_file = stdout;
    if (argc >= 3) {
        output_file = fopen(argv[2], "w");
        if (!output_file) {
            perror("Error opening output file");
            fclose(input_file);
            return 1;
        }
    }

    yyin = input_file;  // Set input file for lexer

    int token_number = 1;
    alpha_token_t token = {0}; // Initialize the token variable

    // Lexer loop: Read tokens from the input
    while (1) {
        int token_type = alpha_yylex(&token);
        if (token_type == 0) { // EOF
            break;
        }
    /*
        // Skip invalid tokens (UNKNOWN) but continue processing valid ones
        if (token.category == UNKNOWN) {
            // Free memory for the invalid token
            if (token.attribute.str_val != NULL) {
                free(token.attribute.str_val);
            }
            continue; // Skip to the next token
        }
    */
        // Print the token
        print_token(yylineno, token_number++, &token);
    
        // Free memory for string attributes
        if (token.category == STRINGCONST || token.category == IDENT ||
            token.category == KEYWORD || token.category == OPERATOR ||
            token.category == PUNCTUATION) {
            if (token.attribute.str_val != NULL) {
                free(token.attribute.str_val);
            }
        }
    }

    fclose(input_file);
    if (output_file != stdout) {
        fclose(output_file);
    }

    return 0;
}