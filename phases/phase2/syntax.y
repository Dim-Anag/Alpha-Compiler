%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "symboltable.h"


extern FILE *yyin;
extern int yylex();
extern void yyerror(const char *s);
extern int yylineno;


SymbolTable *symbol_table; // Global symbol table
int current_scope = 0;     // Start with global scope (scope 0)
int parsing_funcdef = 0; // Global flag for function parsing
int parsing_anon_funcdef = 0; // Flag for parsing anonymous functions
int loop_depth = 0;
%}


%union {
    char charval;
    int intval;
    double doubleval; 
    char *strval;
    struct Symbol *symbol; // Explicitly use struct Symbol
    
}



%type <symbol> vardecl
%type <symbol> lvalue
%type <symbol> assignexpr
%type <symbol> member
%type <symbol> call
%type <symbol> funcdef
%type <symbol> elist
%type <symbol> expr_list
%type <symbol> expr
%type <symbol> term
%type <symbol> primary
%type <symbol> objectdef
%type <symbol> const
%type <symbol> indexed_list
%type <symbol> indexedelem
%type <symbol> type
%type <symbol> block
%type <symbol> stmt_list
%type <symbol> stmt
%type <symbol> funcdecl
%type <symbol> funcbody
%type <symbol> function_block
%type <symbol> anonfuncdef
%type <symbol> anonfuncdecl
%type <symbol> anonfuncbody
%type <symbol> anonfuncdecl_head
%type <symbol> whilestmt
%type <symbol> forstmt
%type <symbol> returnstmt
%type <symbol> return_no_expr
%type <symbol> return_with_expr

%token <strval> IDENT
%token <intval> INTCONST
%token <doubleval> REALCONST
%token <strval> STRINGCONST
%token UNKNOWN

// Specific subcategories for keywords
%token KEYWORD_IF      
%token KEYWORD_ELSE     
%token KEYWORD_WHILE    
%token KEYWORD_FOR      
%token <intval> KEYWORD_FUNCTION
%token KEYWORD_RETURN   
%token KEYWORD_BREAK    
%token KEYWORD_CONTINUE 
%token KEYWORD_AND      
%token KEYWORD_NOT      
%token KEYWORD_OR       
%token KEYWORD_LOCAL    
%token KEYWORD_TRUE    
%token KEYWORD_FALSE    
%token KEYWORD_NIL     
%token KEYWORD_INT 
%token KEYWORD_REAL 

// Specific subcategories for operators
%token OPERATOR_EQUAL       
%token OPERATOR_PLUS        
%token OPERATOR_MINUS       
%token OPERATOR_MULTIPLY    
%token OPERATOR_DIVIDE      
%token OPERATOR_MODULO      
%token OPERATOR_EQUAL_EQUAL 
%token OPERATOR_NOT_EQUAL   
%token OPERATOR_INCREMENT   
%token OPERATOR_DECREMENT   
%token OPERATOR_GREATER    
%token OPERATOR_LESS        
%token OPERATOR_GREATER_EQUAL 
%token OPERATOR_LESS_EQUAL  
%token OPERATOR_UMINUS

// Specific subcategories for punctuation
%token PUNCTUATION_LEFT_PARENTHESIS     
%token PUNCTUATION_RIGHT_PARENTHESIS    
%token PUNCTUATION_LEFT_BRACE          
%token PUNCTUATION_RIGHT_BRACE       
%token PUNCTUATION_LEFT_BRACKET        
%token PUNCTUATION_RIGHT_BRACKET       
%token PUNCTUATION_SEMICOLON        
%token PUNCTUATION_COMMA              
%token PUNCTUATION_COLON              
%token PUNCTUATION_DOUBLE_COLON      
%token PUNCTUATION_DOT                
%token PUNCTUATION_DOUBLE_DOT           

%nonassoc OPERATOR_GREATER OPERATOR_GREATER_EQUAL OPERATOR_LESS OPERATOR_LESS_EQUAL
%nonassoc OPERATOR_EQUAL_EQUAL OPERATOR_NOT_EQUAL
%right KEYWORD_NOT OPERATOR_INCREMENT OPERATOR_DECREMENT OPERATOR_UMINUS OPERATOR_EQUAL
%left OPERATOR_PLUS OPERATOR_MINUS
%left OPERATOR_MULTIPLY OPERATOR_DIVIDE OPERATOR_MODULO
%left KEYWORD_AND
%left KEYWORD_OR
%left PUNCTUATION_COLON
%left PUNCTUATION_LEFT_PARENTHESIS PUNCTUATION_RIGHT_PARENTHESIS
%left PUNCTUATION_LEFT_BRACKET PUNCTUATION_RIGHT_BRACKET
%left PUNCTUATION_DOT PUNCTUATION_DOUBLE_DOT
%left LOWER_THAN_ELSE
%left KEYWORD_ELSE
%left KEYWORD_RETURN
%nonassoc FUNC_PRIORITY

%locations
%token END_OF_FILE        0           

%start program // Add this line here
// %expect 1 //call IDENT
%%

program: stmt_list END_OF_FILE
       | END_OF_FILE; // Allow empty program

stmt_list: stmt_list stmt {
    if (parsing_funcdef || parsing_anon_funcdef) {
        printf("DEBUG: Skipping redundant parsing for function body.\n");
        $$ = $1; // Pass the previous stmt_list without adding the redundant stmt
    } else {
        $$ = $1; // Add the new stmt to the stmt_list
    }
}
| stmt {
    if (parsing_funcdef || parsing_anon_funcdef) {
        printf("DEBUG: Parsing single statement inside function body.\n");
    }
    $$ = $1; // Pass the value of the single stmt
};

stmt:
    funcdef { $$ = $1; }
  | anonfuncdef { $$ = $1; }
  | vardecl PUNCTUATION_SEMICOLON { $$ = $1; }
  | block { $$ = $1; }
  | expr PUNCTUATION_SEMICOLON { $$ = $1; }
  | ifstmt { $$ = NULL; }
  | whilestmt { $$ = NULL; }
  | forstmt { $$ = NULL; }
  | returnstmt PUNCTUATION_SEMICOLON { $$ = NULL; }
  | KEYWORD_BREAK PUNCTUATION_SEMICOLON {
        if (loop_depth == 0) {
            fprintf(stderr, "Error: Use of 'break' while not in a loop at line %d.\n", yylineno);
            exit(1);
        }
        $$ = NULL;
    }
  | KEYWORD_CONTINUE PUNCTUATION_SEMICOLON {
        if (loop_depth == 0) {
            fprintf(stderr, "Error: Use of 'continue' while not in a loop at line %d.\n", yylineno);
            exit(1);
        }
        $$ = NULL;
    }
  | PUNCTUATION_SEMICOLON { $$ = NULL; }
;



vardecl: type IDENT {
              Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1); // Restrict to current scope
             if (existing) {
                 fprintf(stderr, "Error: Redeclaration of variable '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
             if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
                 fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             insert_symbol(symbol_table, $2, VARIABLE, @2.first_line, current_scope);
            // printf("DEBUG: Variable '%s' declared at line %d in scope %d.\n", $2, @2.first_line, current_scope);
         }
       | type IDENT OPERATOR_EQUAL expr {
             Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1);
             if (existing) {
                 fprintf(stderr, "Error: Redeclaration of variable '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
             if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
                 fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             insert_symbol(symbol_table, $2, VARIABLE, @2.first_line, current_scope);
            // printf("DEBUG: Variable '%s' declared at line %d in scope %d.\n", $2, @2.first_line, current_scope);
         };


expr: expr PUNCTUATION_DOUBLE_DOT expr {
         // Handle string concatenation or other operations
         $$ = $1; // Adjust as needed for semantic actions
     }
   | expr op expr %prec OPERATOR_PLUS {
         $$ = $1; // Handle binary operations
     }
   | assignexpr {
         $$ = $1; // Pass the value of assignexpr
     }
   | term {
         $$ = $1; // Pass the value of term
     }
     ;

type: KEYWORD_INT {
    $$ = malloc(sizeof(Symbol));
    $$->type = VARIABLE;
    $$->name = strdup("int");
    $$->line = yylineno;
}
| KEYWORD_REAL {
    $$ = malloc(sizeof(Symbol));
    $$->type = VARIABLE;
    $$->name = strdup("real");
    $$->line = yylineno;
};

op: OPERATOR_PLUS
   | OPERATOR_MINUS
   | OPERATOR_MULTIPLY
   | OPERATOR_DIVIDE
   | OPERATOR_MODULO
   | OPERATOR_GREATER
   | OPERATOR_GREATER_EQUAL
   | OPERATOR_LESS
   | OPERATOR_LESS_EQUAL
   | OPERATOR_EQUAL_EQUAL
   | OPERATOR_NOT_EQUAL
   | KEYWORD_AND
   | KEYWORD_OR
   ;

term: OPERATOR_INCREMENT lvalue %prec OPERATOR_INCREMENT {
          if ($2->type == USER_FUNCTION) {
              fprintf(stderr, "Error: Cannot increment user function '%s' at line %d.\n", $2->name, yylineno);
              exit(1);
          }
          if ($2->type == LIBRARY_FUNCTION) {
              fprintf(stderr, "Error: Cannot increment library function '%s' at line %d.\n", $2->name, yylineno);
              exit(1);
          }
          $$ = $2;
      }
    | OPERATOR_DECREMENT lvalue %prec OPERATOR_DECREMENT {
          if ($2->type == USER_FUNCTION) {
              fprintf(stderr, "Error: Cannot decrement user function '%s' at line %d.\n", $2->name, yylineno);
              exit(1);
          }
          if ($2->type == LIBRARY_FUNCTION) {
              fprintf(stderr, "Error: Cannot decrement library function '%s' at line %d.\n", $2->name, yylineno);
              exit(1);
          }
          $$ = $2;
      }
    | lvalue OPERATOR_INCREMENT %prec OPERATOR_INCREMENT {
          if ($1->type == USER_FUNCTION) {
              fprintf(stderr, "Error: Cannot increment user function '%s' at line %d.\n", $1->name, yylineno);
              exit(1);
          }
          if ($1->type == LIBRARY_FUNCTION) {
              fprintf(stderr, "Error: Cannot increment library function '%s' at line %d.\n", $1->name, yylineno);
              exit(1);
          }
          $$ = $1;
      }
    | lvalue OPERATOR_DECREMENT %prec OPERATOR_DECREMENT {
          if ($1->type == USER_FUNCTION) {
              fprintf(stderr, "Error: Cannot decrement user function '%s' at line %d.\n", $1->name, yylineno);
              exit(1);
          }
          if ($1->type == LIBRARY_FUNCTION) {
              fprintf(stderr, "Error: Cannot decrement library function '%s' at line %d.\n", $1->name, yylineno);
              exit(1);
          }
          $$ = $1;
      }
    | PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS {
          $$ = $2;
      }
    | OPERATOR_MINUS expr %prec OPERATOR_UMINUS {
          $$ = $2;
      }
    | KEYWORD_NOT expr {
          $$ = $2;
      }
    | primary {
          $$ = $1;
      };

assignexpr: lvalue OPERATOR_EQUAL expr {
                Symbol *symbol = lookup_symbol(symbol_table, $1->name, current_scope, 0);
                if (!symbol) {
                    // Only insert if it's not an object member
                    if ($1->type != MEMBER) {
                        insert_symbol(symbol_table, $1->name, VARIABLE, yylineno, current_scope);
                    }
                    symbol = lookup_symbol(symbol_table, $1->name, current_scope, 1);
                } else if (symbol->type == USER_FUNCTION) {
                    fprintf(stderr, "Error: Cannot assign to user function '%s' at line %d.\n", $1->name, yylineno);
                    exit(1);
                } else if (symbol->type == LIBRARY_FUNCTION) {
                    fprintf(stderr, "Error: Cannot assign to library function '%s' at line %d.\n", $1->name, yylineno);
                    exit(1);
                }

                $$ = symbol;
            };

primary: lvalue %prec PUNCTUATION_DOUBLE_DOT {
             $$ = $1; // Pass the value of lvalue
         }
       | call {
             $$ = $1; // Pass the value of call
         }
       | objectdef {
             $$ = $1; // Pass the value of objectdef
         }
       | PUNCTUATION_LEFT_PARENTHESIS funcdef PUNCTUATION_RIGHT_PARENTHESIS {
             // Handle anonymous function
             $$ = $2; // Pass the value of the anonymous function created in funcdef
         }
    | PUNCTUATION_LEFT_PARENTHESIS anonfuncdef PUNCTUATION_RIGHT_PARENTHESIS { $$ = $2; } 
       | const {
             $$ = $1; // Pass the value of const
         };


lvalue: IDENT %prec PUNCTUATION_DOUBLE_DOT {
    Symbol *symbol = lookup_symbol(symbol_table, $1, current_scope, 0);
    if (!symbol) {
        // Allow implicit declaration in all scopes
        insert_symbol(symbol_table, $1, VARIABLE, yylineno, current_scope);
        symbol = lookup_symbol(symbol_table, $1, current_scope, 1); // Retrieve the newly inserted symbol
    } else {
        // --- BEGIN: Check for illegal access to parent function locals/args ---
        if ((parsing_funcdef || parsing_anon_funcdef) &&
            (symbol->type == VARIABLE || symbol->type == FORMAL_ARGUMENT) &&
            symbol->scope != 0 && symbol->scope != current_scope) {
            // Optionally: check if symbol->scope is a function scope, not a block
            fprintf(stderr, "Error: Cannot access local variable or argument '%s' from parent function at line %d.\n", $1, yylineno);
            exit(1);
        }
        // --- END: Check for illegal access ---
    }
    $$ = symbol; // Pass the symbol to the next rule
}
      | KEYWORD_LOCAL IDENT {
            Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1);
            if (existing) {
                fprintf(stderr, "Error: Variable '%s' already declared in scope %d at line %d.\n", $2, current_scope, yylineno);
                exit(1);
            }
             Symbol *lib_conflict = lookup_symbol(symbol_table, $2, 0, 0);
            if (lib_conflict && lib_conflict->type == LIBRARY_FUNCTION) {
                fprintf(stderr, "Error: Cannot shadow library function '%s' with local variable at line %d.\n", $2, yylineno);
                exit(1);
            }
            insert_symbol(symbol_table, $2, VARIABLE, yylineno, current_scope);
            $$ = lookup_symbol(symbol_table, $2, current_scope, 1); // Retrieve the newly inserted symbol
        }
      | PUNCTUATION_DOUBLE_COLON IDENT {
            Symbol *symbol = lookup_symbol(symbol_table, $2, 0, 0); // Check global scope
            if (!symbol) {
                fprintf(stderr, "Error: Undefined global variable or function '%s' at line %d.\n", $2, yylineno);
                exit(1);
            }
            $$ = symbol;
        }
      | member;
      

member: lvalue PUNCTUATION_DOT IDENT {
            if ($1->type != MEMBER && $1->type != VARIABLE && $1->type != FORMAL_ARGUMENT) {
            fprintf(stderr, "Error: Cannot index non-object/non-array '%s' at line %d.\n", $1->name, yylineno);
            exit(1);
        }
            // Do not look up $1->name in the symbol table if $1 is a MEMBER!
            $$ = malloc(sizeof(Symbol));
            $$->type = MEMBER; // New type for indexed members
            $$->name = strdup($3); // Store the actual member name
            $$->line = yylineno;
        }
      | lvalue PUNCTUATION_LEFT_BRACKET expr PUNCTUATION_RIGHT_BRACKET {
            // Handle array or object indexing (e.g., array[index] or object[key])
            Symbol *symbol = lookup_symbol(symbol_table, $1->name, current_scope, 0);
            if (!symbol) {
                fprintf(stderr, "Error: Undefined array or object '%s' at line %d.\n", $1->name, yylineno);
                exit(1);
            }
            // Do not insert the indexed member into the symbol table
            $$ = malloc(sizeof(Symbol));
            $$->type = MEMBER; // New type for indexed members
            $$->name = strdup("indexed_member");
            $$->line = yylineno;
        }
      | call PUNCTUATION_DOT IDENT {
            // Handle method access (e.g., object.method())
            Symbol *symbol = lookup_symbol(symbol_table, $1->name, current_scope, 0);
            if (!symbol) {
                fprintf(stderr, "Error: Undefined object or method '%s' at line %d.\n", $1->name, yylineno);
                exit(1);
            }
            $$ = malloc(sizeof(Symbol));
            $$->type = USER_FUNCTION; // Treat as a function
            $$->name = strdup($3); // The method name
            $$->line = yylineno;
        }
      | call PUNCTUATION_LEFT_BRACKET expr PUNCTUATION_RIGHT_BRACKET {
            // Handle dynamic method or function call (e.g., object[method]())
            $$ = malloc(sizeof(Symbol));
            $$->type = MEMBER; // New type for dynamic calls
            $$->name = strdup("dynamic_call");
            $$->line = yylineno;
        };

call: call PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS {
          // Handle function call
      }
    | lvalue callsuffix {
          // Handle lvalue with callsuffix
      }
    | PUNCTUATION_LEFT_PARENTHESIS funcdef PUNCTUATION_RIGHT_PARENTHESIS PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS {
          // Handle anonymous function call
      };
      
callsuffix: normcall
          | methodcall;

normcall: PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS {
              // Handle normal function call
          };

methodcall: lvalue PUNCTUATION_DOUBLE_DOT IDENT PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS;

          
elist: /* empty */ {
           $$ = NULL; // Set $$ to NULL for the empty case
       }
     | expr_list {
           $$ = $1; // Pass the value of expr_list
       };

expr_list: expr {
               $$ = $1; // Pass the value of expr
           }
         | expr_list PUNCTUATION_COMMA expr {
               // Handle a list of expressions
               // You can create a linked list or array of symbols here
               $$ = $1; // For now, just pass the first expression
           };

objectdef: PUNCTUATION_LEFT_BRACKET expr_list PUNCTUATION_RIGHT_BRACKET {
               $$ = $2; // Pass the value of expr_list
           }
         | PUNCTUATION_LEFT_BRACKET indexed_list PUNCTUATION_RIGHT_BRACKET {
               $$ = $2; // Pass the value of indexed_list
           }
         | PUNCTUATION_LEFT_BRACKET PUNCTUATION_RIGHT_BRACKET {
               $$ = NULL; // Handle empty object
           };


indexed_list: indexedelem {
                  $$ = $1; // Pass the value of indexedelem
              }
            | indexed_list PUNCTUATION_COMMA indexedelem {
                  // Handle a list of indexed elements
                  $$ = $1; // For now, just pass the first indexed element
              };

indexedelem: PUNCTUATION_LEFT_BRACE expr PUNCTUATION_COLON expr PUNCTUATION_RIGHT_BRACE {
                 $$ = malloc(sizeof(Symbol)); // Create a new symbol for the indexed element
                 $$->type = VARIABLE;         // Treat indexed elements as variables
                 $$->name = strdup("indexedelem");
                 $$->line = yylineno;
             };



funcdef: funcdecl funcbody {
    $$ = $2;
    printf("DEBUG: Function definition completed for '%s' at line %d in scope %d.\n", $1->name, $1->line, current_scope);

    parsing_funcdef--; // DECREMENT after parsing the function body

    deactivate_scope(symbol_table, current_scope);

    printf("DEBUG: Exiting function body scope %d at line %d.\n", current_scope, yylineno);
    current_scope--;
};

funcdecl: KEYWORD_FUNCTION IDENT PUNCTUATION_LEFT_PARENTHESIS {
    Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1);
    if (existing) {
        fprintf(stderr, "Error: Function '%s' already declared in scope %d at line %d.\n", $2, current_scope, yylineno);
        exit(1);
    }

    Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
    if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, yylineno);
        exit(1);
    }

    insert_symbol(symbol_table, $2, USER_FUNCTION, yylineno, current_scope);

    current_scope++;
    parsing_funcdef++; // INCREMENT for nested functions!
    printf("DEBUG: Entering function body scope %d for function '%s' at line %d.\n", current_scope, $2, yylineno);
}
idlist PUNCTUATION_RIGHT_PARENTHESIS {
    printf("DEBUG: Parsing function declaration '%s' at line %d in scope %d.\n", $2, yylineno, current_scope);

    $$ = malloc(sizeof(Symbol));
    $$->name = strdup($2);
    $$->type = USER_FUNCTION;
    $$->line = yylineno;
    $$->scope = current_scope - 1;
};

funcbody: function_block {
    $$ = $1;
    printf("DEBUG: Function body parsed.\n");
};

function_block: PUNCTUATION_LEFT_BRACE stmt_list PUNCTUATION_RIGHT_BRACE {
    $$ = $2;
    printf("DEBUG: Function body parsed.\n");
}
| PUNCTUATION_LEFT_BRACE PUNCTUATION_RIGHT_BRACE {
    if (parsing_anon_funcdef) {
        printf("DEBUG: Empty anonymous function body at line %d.\n", yylineno);
    } else {
        printf("DEBUG: Empty named function body or standalone block at line %d.\n", yylineno);
    }
    $$ = NULL;
};

anonfuncdef: anonfuncdecl anonfuncbody {
    $$ = $2;
    printf("DEBUG: Anonymous function definition completed for '%s' at line %d in scope %d.\n", $1->name, $1->line, current_scope);

    parsing_anon_funcdef--; // DECREMENT after parsing the function body

    deactivate_scope(symbol_table, current_scope);

    printf("DEBUG: Exiting anonymous function body scope %d at line %d.\n", current_scope, yylineno);
    current_scope--;
};

anonfuncdecl: anonfuncdecl_head idlist PUNCTUATION_RIGHT_PARENTHESIS {
    $$ = $1;
}
anonfuncdecl_head: KEYWORD_FUNCTION PUNCTUATION_LEFT_PARENTHESIS {
    parsing_anon_funcdef++; // INCREMENT for nested anonymous functions

    static int anonymous_function_counter = 0;
    char *anon_name = malloc(64);
    snprintf(anon_name, 64, "anonymous_function_%d", ++anonymous_function_counter);

    printf("DEBUG: Declaring anonymous function '%s' at line %d in scope %d.\n", anon_name, yylineno, current_scope);

    Symbol *global_conflict = lookup_symbol(symbol_table, anon_name, 0, 0);
    if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", anon_name, yylineno);
        exit(1);
    }

    insert_symbol(symbol_table, anon_name, USER_FUNCTION, yylineno, current_scope);
    printf("DEBUG: Inserted anonymous function '%s' into symbol table at line %d in scope %d.\n", anon_name, yylineno, current_scope);

    current_scope++;
    printf("DEBUG: Entering anonymous function body scope %d at line %d.\n", current_scope, yylineno);

    $$ = malloc(sizeof(Symbol));
    $$->type = USER_FUNCTION;
    $$->name = anon_name;
    $$->line = yylineno;
    $$->scope = current_scope - 1;
};

anonfuncbody: function_block {
    $$ = $1;
    printf("DEBUG: Anonymous function body parsed.\n");
};




block: block_start stmt_list block_end {
    printf("DEBUG: Parsing block with statements.\n");
    $$ = $2; // Pass the value of stmt_list
}
| block_start block_end {
    if (parsing_anon_funcdef) {
        printf("DEBUG: Empty anonymous function body at line %d.\n", yylineno);
    } else {
        printf("DEBUG: Empty standalone block at line %d.\n", yylineno);
    }
    $$ = NULL; // Handle empty block
};

block_start: PUNCTUATION_LEFT_BRACE {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        current_scope++; // Increment scope for standalone blocks
        printf("DEBUG: Entering block scope %d at line %d.\n", current_scope, yylineno);
    } else {
        printf("DEBUG: Skipping scope increment for function body at line %d.\n", yylineno);
    }
};

block_end: PUNCTUATION_RIGHT_BRACE {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        printf("DEBUG: Closing standalone block at line %d.\n", yylineno);
        current_scope--; // <-- Make sure this is here!
    } else if (parsing_anon_funcdef) {
        printf("DEBUG: Closing anonymous function body at line %d.\n", yylineno);
    } else if (parsing_funcdef) {
        printf("DEBUG: Closing named function body at line %d.\n", yylineno);
    }
};





const: INTCONST {
           $$ = malloc(sizeof(Symbol)); // Create a new symbol for the constant
           $$->type = VARIABLE;         // Treat constants as variables
           $$->name = strdup("intconst");
           $$->line = yylineno;
       }
     | REALCONST {
           $$ = malloc(sizeof(Symbol));
           $$->type = VARIABLE;
           $$->name = strdup("realconst");
           $$->line = yylineno;
       }
     | STRINGCONST {
           $$ = malloc(sizeof(Symbol));
           $$->type = VARIABLE;
           $$->name = strdup("stringconst");
           $$->line = yylineno;
       }
     | KEYWORD_NIL {
           $$ = malloc(sizeof(Symbol));
           $$->type = VARIABLE;
           $$->name = strdup("nil");
           $$->line = yylineno;
       }
     | KEYWORD_TRUE {
           $$ = malloc(sizeof(Symbol));
           $$->type = VARIABLE;
           $$->name = strdup("true");
           $$->line = yylineno;
       }
     | KEYWORD_FALSE {
           $$ = malloc(sizeof(Symbol));
           $$->type = VARIABLE;
           $$->name = strdup("false");
           $$->line = yylineno;
       };

idlist: /* empty */
      | idlist PUNCTUATION_COMMA IDENT {
            // Determine the correct scope for formal arguments
            int argument_scope = current_scope; // Always use current_scope for both named and anonymous functions

            Symbol *existing = lookup_symbol(symbol_table, $3, argument_scope, 1);
            if (existing) {
                fprintf(stderr, "Error: Formal argument '%s' already declared in scope %d at line %d.\n", $3, argument_scope, yylineno);
                exit(1);
            }
            Symbol *lib_conflict = lookup_symbol(symbol_table, $3, 0, 0);
            if (lib_conflict && lib_conflict->type == LIBRARY_FUNCTION) {
                fprintf(stderr, "Error: Cannot use library function name '%s' as a formal argument at line %d.\n", $3, yylineno);
                exit(1);
            }

            // Insert the formal argument into the symbol table
            insert_symbol(symbol_table, $3, FORMAL_ARGUMENT, yylineno, argument_scope);
            printf("DEBUG: Inserting formal argument '%s' into symbol table at line %d in scope %d.\n", $3, yylineno, argument_scope);
        }
      | IDENT {
            // Determine the correct scope for formal arguments
            int argument_scope = current_scope; // Always use current_scope for both named and anonymous functions

            Symbol *existing = lookup_symbol(symbol_table, $1, argument_scope, 1);
            if (existing) {
                fprintf(stderr, "Error: Formal argument '%s' already declared in scope %d at line %d.\n", $1, argument_scope, yylineno);
                exit(1);
            }
            Symbol *lib_conflict = lookup_symbol(symbol_table, $1, 0, 0);
            if (lib_conflict && lib_conflict->type == LIBRARY_FUNCTION) {
                fprintf(stderr, "Error: Cannot use library function name '%s' as a formal argument at line %d.\n", $1, yylineno);
                exit(1);
            }

            // Insert the formal argument into the symbol table
            insert_symbol(symbol_table, $1, FORMAL_ARGUMENT, yylineno, argument_scope);
            printf("DEBUG: Inserting formal argument '%s' into symbol table at line %d in scope %d.\n", $1, yylineno, argument_scope);
        };

ifstmt: if_no_else
      | if_with_else
      ;

if_no_else: KEYWORD_IF PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS stmt;

if_with_else: KEYWORD_IF PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS stmt KEYWORD_ELSE stmt %prec LOWER_THAN_ELSE;

whilestmt:
    KEYWORD_WHILE PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS {
        loop_depth++;
    } stmt {
        loop_depth--;
        $$ = (struct Symbol*)NULL;
    };

forstmt:
    KEYWORD_FOR PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_SEMICOLON expr PUNCTUATION_SEMICOLON elist PUNCTUATION_RIGHT_PARENTHESIS {
        loop_depth++;
    } stmt {
        loop_depth--;
        $$ = (struct Symbol*)NULL;
    };
returnstmt: return_no_expr
          | return_with_expr;

return_no_expr: KEYWORD_RETURN %prec KEYWORD_RETURN {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        fprintf(stderr, "Error: Use of 'return' while not in a function at line %d.\n", yylineno);
        exit(1);
    }
};
return_with_expr: KEYWORD_RETURN expr {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        fprintf(stderr, "Error: Use of 'return' while not in a function at line %d.\n", yylineno);
        exit(1);
    }
};


%%


// Utility functions

void yyerror(const char *s) {
    extern char *yytext; // Current token text
    extern int yylineno; // Current line number

    fprintf(stderr, "Error: %s at line %d, near '%s'.\n", s, yylineno, yytext);
}



extern int yydebug;

int main(int argc, char **argv) {
    // Initialize the symbol table
    symbol_table = create_symbol_table();
    initialize_library_functions(symbol_table); // Add library functions

    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            perror("Error opening input file");
            return 1;
        }
    }

    // Enable debugging if needed
    // yydebug = 1;

    if (yyparse() == 0) {
        printf("Parsing completed successfully.\n");
    } else {
        printf("Parsing failed.\n");
    }

    // Print the symbol table after parsing
    print_symbol_table(symbol_table);

    // Destroy the symbol table to free memory
    destroy_symbol_table(symbol_table);

    if (yyin) {
        fclose(yyin);
    }

    return 0;
}