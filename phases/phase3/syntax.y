%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "symboltable.h"
#include "quads.h"
#include "expr.h"
#include "temp.h"
#include "scopespace.h"
#include "funcutils.h"
#include "vartempmap.h"


extern FILE *yyin;
extern int yylex();
extern void yyerror(const char *s);
extern int yylineno;
extern quad* quads;
extern void enterscopespace(void);
extern void exitscopespace(void);
extern void resetformalargsoffset(void);
extern void resetfunctionlocalsoffset(void);
extern int currscopeoffset(void);
extern void restorecurrscopeoffset(int offset);
extern void push(void* stack, int value);
extern int pop_and_top(void* stack);
extern void* scopeoffsetstack;
extern unsigned curr_quad;



SymbolTable *symbol_table; // Global symbol table
int current_scope = 0;     // Start with global scope (scope 0)
int parsing_funcdef = 0; // Global flag for function parsing
int parsing_anon_funcdef = 0; // Flag for parsing anonymous functions
int loop_depth = 0;
struct expr* to_bool(struct expr* e);
static unsigned while_test_quad;
static expr* while_cond;
static unsigned while_test_quad;
static unsigned while_jump_to_exit;
static unsigned while_exit;
int is_relational(struct expr* e);
int func_entry_jump = -1;
struct expr* emit_iftableitem(struct expr* e);



quadlist* breaklist_stack[32];
int breaklist_stack_top = -1;
quadlist* continuelist_stack[32];
int continuelist_stack_top = -1;




%}


%code requires {
typedef struct {
    unsigned test;
    unsigned enter;
    unsigned ifeq_quad;
    unsigned jump_out_quad;
} forprefix_t;
/*
typedef struct exprlist {
    struct expr* expr;
    struct exprlist* next;
} exprlist; 
*/


}
%union {
    char charval;
    int intval;
    double doubleval; 
    char *strval;
    struct Symbol *symbol; // Explicitly use struct Symbol
    struct expr *expr;  
    struct exprlist *exprlist;
    forprefix_t forprefix;
    struct quadlist* quadlist;
}



%type <symbol> vardecl
%type <expr> lvalue
%type <expr> assignexpr
// %type <expr> member
%type <expr> call
%type <expr> funcdef
%type <expr> expr
%type <expr> term
%type <expr> primary
%type <expr> objectdef
%type <expr> const
%type <exprlist> indexed_list
%type <expr> indexedelem
%type <symbol> type
%type <expr> block
%type <expr> stmt_list
%type <expr> stmt
%type <symbol> funcdecl
%type <intval> funcbody
%type <expr> function_block
%type <expr> anonfuncdef
%type <symbol> anonfuncdecl
%type <expr> anonfuncbody
%type <symbol> anonfuncdecl_head
%type <expr> whilestmt
%type <expr> loopstmt
%type <intval> whilestart 
%type <expr> whilecond
%type <intval> forstmt
%type <symbol> returnstmt
%type <symbol> return_no_expr
%type <symbol> return_with_expr
%type <intval> ifprefix
%type <intval> elseprefix
%type <intval> N1 N2 N3 
%type <intval> M
%type <forprefix> forprefix
%type <symbol> funcdecl_head
%type <exprlist> elist expr_list



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


%right OPERATOR_INCREMENT OPERATOR_DECREMENT OPERATOR_EQUAL
%left OPERATOR_PLUS OPERATOR_MINUS
%left OPERATOR_MULTIPLY OPERATOR_DIVIDE OPERATOR_MODULO
%right OPERATOR_UMINUS
%left KEYWORD_OR
%left KEYWORD_AND
%nonassoc OPERATOR_GREATER OPERATOR_GREATER_EQUAL OPERATOR_LESS OPERATOR_LESS_EQUAL
%nonassoc OPERATOR_EQUAL_EQUAL OPERATOR_NOT_EQUAL
%right KEYWORD_NOT
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

stmt_list:
      stmt_list stmt {
          if ($1 && $2) {
              stmt_t* merged = malloc(sizeof(stmt_t));
              make_stmt(merged);
              merged->breaklist = merge(((stmt_t*)$1)->breaklist, ((stmt_t*)$2)->breaklist);
              merged->contlist = merge(((stmt_t*)$1)->contlist, ((stmt_t*)$2)->contlist);
              $$ = (void*)merged;
          } else if ($1) {
              $$ = $1;
          } else if ($2) {
              $$ = $2;
          } else {
              $$ = NULL;
          }
      }
    | stmt {
          $$ = $1;
      }
;

stmt:
    funcdef { $$ = $1; }
  | anonfuncdef { $$ = $1; }
  | vardecl PUNCTUATION_SEMICOLON {  
    // Create an expr for the declared variable
    expr* e = newexpr(var_e);
    e->sym = $1;
    $$ = e; }
  | block { $$ = $1; }
  | expr PUNCTUATION_SEMICOLON {
    if ($1 && (
        $1->type == boolexpr_e ||
        $1->type == and_e ||
        $1->type == or_e ||
        $1->type == relop_e ||
        $1->type == not_e
    )) {
        $$ = bool_to_temp(to_bool($1));
    } else {
        $$ = $1;
    }
    }
  | ifstmt { $$ = NULL; }
  | whilestmt { $$ = NULL; }
  | forstmt { $$ = NULL; }
  | returnstmt PUNCTUATION_SEMICOLON { $$ = NULL; }
  | KEYWORD_BREAK PUNCTUATION_SEMICOLON {
    if (loop_depth == 0) {
        fprintf(stderr, "Error: Use of 'break' while not in a loop at line %d.\n", yylineno);
        exit(1);
    }
    quadlist* l = makelist(nextquad());
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    // Add to current breaklist on stack
    breaklist_stack[breaklist_stack_top] = merge(breaklist_stack[breaklist_stack_top], l);
    $$ = NULL;
}
| KEYWORD_CONTINUE PUNCTUATION_SEMICOLON {
    if (loop_depth == 0) {
        fprintf(stderr, "Error: Use of 'continue' while not in a loop at line %d.\n", yylineno);
        exit(1);
    }
    quadlist* l = makelist(nextquad());
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    // Add to current continuelist on stack
    continuelist_stack[continuelist_stack_top] = merge(continuelist_stack[continuelist_stack_top], l);
    $$ = NULL;
}
  | PUNCTUATION_SEMICOLON {
    stmt_t* s = malloc(sizeof(stmt_t));
    make_stmt(s);
    $$ = (void*)s;
}



vardecl: type IDENT {
              Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1); // Restrict to current scope
             if (existing) {
                 fprintf(stderr, "Error: Redeclaration of variable '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }
            // Prevent shadowing of library functions in the global scope
             Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
             if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
                 fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             insert_symbol(symbol_table, $2, VARIABLE, @2.first_line, current_scope);
            // printf("DEBUG: Variable '%s' declared at line %d in scope %d.\n", $2, @2.first_line, current_scope);
         }
       | type IDENT OPERATOR_EQUAL expr {
            // Check for redeclaration in the current scope
             Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1);
             if (existing) {
                 fprintf(stderr, "Error: Redeclaration of variable '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }
            // Prevent shadowing of library functions in the global scope
             Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
             if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
                 fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, @2.first_line);
                 exit(1);
             }

             insert_symbol(symbol_table, $2, VARIABLE, @2.first_line, current_scope);
            // printf("DEBUG: Variable '%s' declared at line %d in scope %d.\n", $2, @2.first_line, current_scope);
         };


expr:  KEYWORD_LOCAL IDENT OPERATOR_EQUAL expr {
      // Declare a new local variable and assign a value
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
        Symbol *symbol = lookup_symbol(symbol_table, $2, current_scope, 1);
        expr* e = newexpr(var_e);
        e->sym = symbol;
        emit(assign, e, $4, NULL, 0, yylineno);
        $$ = e;
    }
    | expr OPERATOR_EQUAL_EQUAL expr {
        expr* rel = newexpr(relop_e);
        rel->op = if_eq;
        rel->left = $1;
        rel->right = $3;
        $$ = rel;
    }
  | expr OPERATOR_NOT_EQUAL expr {
    expr* rel = newexpr(relop_e);
    rel->op = if_noteq;
    rel->left = $1;
    rel->right = $3;
    $$ = rel;
}
  | expr OPERATOR_GREATER expr {
    expr* rel = newexpr(relop_e);
    rel->op = if_greater;
    rel->left = $1;
    rel->right = $3;
    $$ = rel;
}
  | expr OPERATOR_GREATER_EQUAL expr {
    expr* rel = newexpr(relop_e);
    rel->op = if_greatereq;
    rel->left = $1;
    rel->right = $3;
    $$ = rel;
}
  | expr OPERATOR_LESS expr {
    expr* rel = newexpr(relop_e);
    rel->op = if_less;
    rel->left = $1;
    rel->right = $3;
    $$ = rel;
}
  | expr OPERATOR_LESS_EQUAL expr {
    expr* rel = newexpr(relop_e);
    rel->op = if_lesseq;
    rel->left = $1;
    rel->right = $3;
    $$ = rel;
}
  | expr KEYWORD_AND expr {
    expr* e = newexpr(and_e);
    e->left = $1;
    e->right = $3;
    $$ = e;
}
| expr KEYWORD_OR expr {
    expr* e = newexpr(or_e);
    e->left = $1;
    e->right = $3;
    $$ = e;
}
| expr OPERATOR_PLUS expr {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(add, temp, $1, $3, 0, yylineno);
    $$ = temp;
}
| OPERATOR_MINUS expr %prec OPERATOR_UMINUS {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(uminus, temp, $2, NULL, 0, yylineno);
    $$ = temp;
}
| expr OPERATOR_MINUS expr {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(sub, temp, $1, $3, 0, yylineno);
    $$ = temp;
}
| expr OPERATOR_MULTIPLY expr {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(mul, temp, $1, $3, 0, yylineno);
    $$ = temp;
}
| expr OPERATOR_DIVIDE expr {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(div_, temp, $1, $3, 0, yylineno);
    $$ = temp;
}
| expr OPERATOR_MODULO expr {
    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(mod, temp, $1, $3, 0, yylineno);
    $$ = temp;
}
| expr PUNCTUATION_DOUBLE_DOT IDENT {
    expr* ti = newexpr(tableitem_e);
    ti->sym = $1->sym;
    ti->index = newexpr_conststring($3);
    $$ = ti;
}
| KEYWORD_NOT expr {
    expr* e = $2;
    expr* not_expr = newexpr(not_e);
    not_expr->not_expr = e; 
    $$ = not_expr;
}
| assignexpr { $$ = $1; }
| term {
      $$ = $1;
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




term: OPERATOR_INCREMENT lvalue %prec OPERATOR_INCREMENT {
    // ++x
    if ($2->sym->type == USER_FUNCTION) {
        fprintf(stderr, "Error: Cannot increment user function '%s' at line %d.\n", $2->sym->name, yylineno);
        exit(1);
    }
    if ($2->sym->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot increment library function '%s' at line %d.\n", $2->sym->name, yylineno);
        exit(1);
    }

    expr* val = emit_iftableitem($2); 
    expr* result = newexpr(var_e);
    result->sym = newtemp();
    emit(add, result, val, newexpr_constnum(1), 0, yylineno);

    if ($2->type == tableitem_e) {
        expr* table = newexpr(var_e); table->sym = $2->sym;
        emit(tablesetelem, table, $2->index, result, 0, yylineno);
    } else {
        emit(assign, $2, result, NULL, 0, yylineno);
    }

    $$ = result;
}
| OPERATOR_DECREMENT lvalue %prec OPERATOR_DECREMENT {
     // --x
    if ($2->sym->type == USER_FUNCTION) {
        fprintf(stderr, "Error: Cannot decrement user function '%s' at line %d.\n", $2->sym->name, yylineno);
        exit(1);
    }
    if ($2->sym->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot decrement library function '%s' at line %d.\n", $2->sym->name, yylineno);
        exit(1);
    }

    expr* val = emit_iftableitem($2);
    expr* result = newexpr(var_e);
    result->sym = newtemp();
    emit(sub, result, val, newexpr_constnum(1), 0, yylineno);

    if ($2->type == tableitem_e) {
        expr* table = newexpr(var_e); table->sym = $2->sym;
        emit(tablesetelem, table, $2->index, result, 0, yylineno);
    } else {
        emit(assign, $2, result, NULL, 0, yylineno);
    }

    $$ = result;
}
| lvalue OPERATOR_INCREMENT %prec OPERATOR_INCREMENT {
    // x++
    if ($1->type == tableitem_e) {
        expr* table = newexpr(var_e); table->sym = $1->sym;
        // old value first
        expr* oldval = newexpr(var_e); oldval->sym = newtemp();
        emit(tablegetelem, oldval, table, $1->index, 0, yylineno);

        // krata old balue se temp gia expr
        expr* temp = newexpr(var_e); temp->sym = newtemp();
        emit(assign, temp, oldval, NULL, 0, yylineno);
        // kanei prwta assign meta add
        expr* newval = newexpr(var_e); newval->sym = newtemp();
        emit(add, newval, oldval, newexpr_constnum(1), 0, yylineno);
        emit(tablesetelem, table, $1->index, newval, 0, yylineno);
        $$ = temp; //return old value
    } else {
        if ($1->sym->type == USER_FUNCTION) {
            fprintf(stderr, "Error: Cannot increment user function '%s' at line %d.\n", $1->sym->name, yylineno);
            exit(1);
        }
        if ($1->sym->type == LIBRARY_FUNCTION) {
            fprintf(stderr, "Error: Cannot increment library function '%s' at line %d.\n", $1->sym->name, yylineno);
            exit(1);
        }
       
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        emit(assign, temp, $1, NULL, 0, yylineno);
        emit(add, $1, $1, newexpr_constnum(1), 0, yylineno);
        $$ = temp;
    }
}
| lvalue OPERATOR_DECREMENT %prec OPERATOR_DECREMENT {
    // x--
    if ($1->type == tableitem_e) {
        expr* table = newexpr(var_e); table->sym = $1->sym;

        expr* oldval = newexpr(var_e); oldval->sym = newtemp();
        emit(tablegetelem, oldval, table, $1->index, 0, yylineno);

       
        expr* temp = newexpr(var_e); temp->sym = newtemp();
        emit(assign, temp, oldval, NULL, 0, yylineno);

        
        expr* newval = newexpr(var_e); newval->sym = newtemp();
        emit(sub, newval, oldval, newexpr_constnum(1), 0, yylineno);

       
        emit(tablesetelem, table, $1->index, newval, 0, yylineno);

        
        $$ = temp;
    } else {
        if ($1->sym->type == USER_FUNCTION) {
            fprintf(stderr, "Error: Cannot decrement user function '%s' at line %d.\n", $1->sym->name, yylineno);
            exit(1);
        }
        if ($1->sym->type == LIBRARY_FUNCTION) {
            fprintf(stderr, "Error: Cannot decrement library function '%s' at line %d.\n", $1->sym->name, yylineno);
            exit(1);
        }
       
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        emit(assign, temp, $1, NULL, 0, yylineno);
       
        emit(sub, $1, $1, newexpr_constnum(1), 0, yylineno);
        $$ = temp;
    }
}
| PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS { $$ = $2; }
| primary { $$ = $1; }
;

assignexpr : lvalue OPERATOR_EQUAL expr {
    expr* lval_expr = $1;
    expr* rval_expr = $3;

    // If rval_expr is any boolean/lazy boolean, convert to temp and emit quads
    if (rval_expr &&
    (rval_expr->type == boolexpr_e ||
     rval_expr->type == and_e ||
     rval_expr->type == or_e ||
     rval_expr->type == relop_e ||
     rval_expr->type == not_e)) {
    rval_expr = bool_to_temp(to_bool(rval_expr));
    }
    if (lval_expr->type == tableitem_e) {
        expr* table = newexpr(var_e); table->sym = lval_expr->sym;
        emit(tablesetelem, table, lval_expr->index, rval_expr, 0, yylineno);

       
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        emit(tablegetelem, temp, table, lval_expr->index, 0, yylineno);
        $$ = temp;
    } else {
        Symbol *symbol = lookup_symbol(symbol_table, lval_expr->sym->name, current_scope, 0);
        if (!symbol) {
            if (lval_expr->sym->type != MEMBER) {
                insert_symbol(symbol_table, lval_expr->sym->name, VARIABLE, yylineno, current_scope);
            }
            symbol = lookup_symbol(symbol_table, lval_expr->sym->name, current_scope, 1);
        } else if (symbol->type == USER_FUNCTION) {
            fprintf(stderr, "Error: Cannot assign to user function '%s' at line %d.\n", lval_expr->sym->name, yylineno);
            exit(1);
        } else if (symbol->type == LIBRARY_FUNCTION) {
            fprintf(stderr, "Error: Cannot assign to library function '%s' at line %d.\n", lval_expr->sym->name, yylineno);
            exit(1);
        }

        emit(assign, lval_expr, rval_expr, NULL, 0, yylineno);

        //create temp perna timi
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        emit(assign, temp, lval_expr, NULL, 0, yylineno);
        $$ = temp;
    }
};

primary: lvalue %prec PUNCTUATION_DOUBLE_DOT { $$ = emit_iftableitem($1); }
       | call { $$ = $1; }
      // | member  { $$ = $1; }
       | objectdef { $$ = $1; }
       | PUNCTUATION_LEFT_PARENTHESIS funcdef PUNCTUATION_RIGHT_PARENTHESIS { $$ = $2; }
       | PUNCTUATION_LEFT_PARENTHESIS anonfuncdef PUNCTUATION_RIGHT_PARENTHESIS { $$ = $2; }
       | const { $$ = $1; }
;


lvalue: IDENT %prec PUNCTUATION_DOUBLE_DOT {
    Symbol *symbol = lookup_symbol(symbol_table, $1, current_scope, 0);
    if (!symbol) {
        insert_symbol(symbol_table, $1, VARIABLE, yylineno, current_scope);
        symbol = lookup_symbol(symbol_table, $1, current_scope, 1);
    } else {
        if ((parsing_funcdef || parsing_anon_funcdef) &&
            (symbol->type == VARIABLE || symbol->type == FORMAL_ARGUMENT) &&
            symbol->scope != 0 && symbol->scope != current_scope) {
            fprintf(stderr, "Error: Cannot access local variable or argument '%s' from parent function at line %d.\n", $1, yylineno);
            exit(1);
        }
    }
    expr* e = newexpr(var_e);
    e->sym = symbol;
    $$ = e;
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
    Symbol *symbol = lookup_symbol(symbol_table, $2, current_scope, 1);
    expr* e = newexpr(var_e);
    e->sym = symbol;

    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(assign, temp, e, NULL, 0, yylineno);
    $$ = temp;
}
| PUNCTUATION_DOUBLE_COLON IDENT {
    Symbol *symbol = lookup_symbol(symbol_table, $2, 0, 0); // Check global scope
    if (!symbol) {
        fprintf(stderr, "Error: Undefined global variable or function '%s' at line %d.\n", $2, yylineno);
        exit(1);
    }
    expr* e = newexpr(var_e);
    e->sym = symbol;

    expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    emit(assign, temp, e, NULL, 0, yylineno);
    $$ = temp;
}
| lvalue PUNCTUATION_DOUBLE_DOT IDENT PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS {
    expr* ti = newexpr(tableitem_e);
    ti->sym = $1->sym;
    ti->index = newexpr_conststring($3);
    $$ = make_call(ti, $5); // $5 is the argument list
}
| lvalue PUNCTUATION_DOUBLE_DOT IDENT {
    expr* ti = newexpr(tableitem_e);
    ti->sym = $1->sym;
    ti->index = newexpr_conststring($3);
    $$ = ti;
}
| lvalue PUNCTUATION_DOT IDENT {
    expr* lv = emit_iftableitem($1);
    expr* ti = newexpr(tableitem_e);
    ti->sym = lv->sym;
    ti->index = newexpr_conststring($3);
    $$ = ti;
}
| lvalue PUNCTUATION_LEFT_BRACKET expr PUNCTUATION_RIGHT_BRACKET {
    expr* lv = emit_iftableitem($1);
    expr* ti = newexpr(tableitem_e);
    ti->sym = lv->sym;
    ti->index = $3;
    $$ = ti;
}
;
      
/*
member
    : primary PUNCTUATION_DOT IDENT {
        expr* e = newexpr(member_e);
        e->sym = malloc(sizeof(Symbol));
        e->sym->type = MEMBER;
        e->sym->name = strdup($3);
        e->sym->line = yylineno;
        e->index = $1; // Κράτα reference στο object/προηγούμενο expr
        $$ = e;
    }
    | primary PUNCTUATION_LEFT_BRACKET expr PUNCTUATION_RIGHT_BRACKET {
        expr* e = newexpr(member_e);
        e->sym = malloc(sizeof(Symbol));
        e->sym->type = MEMBER;
        e->sym->name = strdup("indexed_member");
        e->sym->line = yylineno;
        e->index = $1; // Κράτα reference στο object/προηγούμενο expr
        $$ = e;
    }
;
*/

call
    : primary PUNCTUATION_LEFT_PARENTHESIS elist PUNCTUATION_RIGHT_PARENTHESIS {
        expr* func = emit_iftableitem($1);

        // Count the number of parameters
        exprlist* l = $3;
        int count = 0;
        for (exprlist* tmp = l; tmp; tmp = tmp->next) count++;

        // Store parameters in an array to emit them in the correct order
        expr** params = malloc(count * sizeof(expr*));
        int i = count - 1;
        for (exprlist* tmp = l; tmp; tmp = tmp->next, i--) params[i] = tmp->expr;

         // Emit param quads from last to first (so the first parameter is pushed last)
        for (i = count - 1; i >= 0; i--) emit(param, params[i], NULL, NULL, 0, yylineno);
        free(params);

        emit(call, func, NULL, NULL, 0, yylineno);
        expr* temp = newexpr(var_e);
        temp->sym = newtemp();
        emit(getretval, temp, NULL, NULL, 0, yylineno);
        $$ = temp;
    }
;

          
elist: /* empty */ {
           $$ = NULL; // Set $$ to NULL for the empty case
       }
     | expr_list {
           $$ = $1; // Pass the value of expr_list
       };

expr_list
    : expr {
        exprlist* l = malloc(sizeof(exprlist));
        l->expr = $1; l->next = NULL;
        $$ = l;
    }
    | expr_list PUNCTUATION_COMMA expr {
        exprlist* l = malloc(sizeof(exprlist));
        l->expr = $3; l->next = $1;
        $$ = l;
    }
;

objectdef
    : PUNCTUATION_LEFT_BRACKET expr_list PUNCTUATION_RIGHT_BRACKET {
        // Array-like table: create a new table and fill it with values from expr_list
        expr* e = newexpr(newtable_e);
        e->elist = $2;
        e->sym = newtemp();
        emit(tablecreate, e, NULL, NULL, 0, yylineno);
      //  printf("DEBUG: tablecreate for temp %s at line %d\n", e->sym->name, yylineno);
         // Reverse the exprlist to preserve source order (since expr_list is built right-to-left)
        exprlist* reversed = reverse_exprlist($2); // <-- ΕΔΩ
        int idx = 0;
        for (exprlist* l = reversed; l; l = l->next, idx++) {
            expr* index_expr = newexpr(constnum_e);
            index_expr->numConst = idx;
         //   printf("DEBUG: tablesetelem %s %d expr_type=%d\n", e->sym->name, idx, l->expr ? l->expr->type : -1);
            emit(tablesetelem, e, index_expr, l->expr, 0, yylineno);
        }

        $$ = e;
    }
    | PUNCTUATION_LEFT_BRACKET indexed_list PUNCTUATION_RIGHT_BRACKET {
        // Map-like table: create a new table and fill it with key-value pairs from indexed_list
    expr* e = newexpr(newtable_e);
    e->elist = $2;
    e->sym = newtemp();
    emit(tablecreate, e, NULL, NULL, 0, yylineno);
    // Reverse
    exprlist* reversed = reverse_exprlist($2);
    for (exprlist* l = reversed; l; l = l->next) {
        emit(tablesetelem, e, l->expr->index, l->expr, 0, yylineno);
    }

    $$ = e;
    }
    | PUNCTUATION_LEFT_BRACKET PUNCTUATION_RIGHT_BRACKET {
        // Empty table
        expr* e = newexpr(newtable_e);
        e->elist = NULL;
        e->sym = newtemp();
        emit(tablecreate, e, NULL, NULL, 0, yylineno);
        $$ = e;
    }
;

indexed_list
    : indexedelem {
        exprlist* l = malloc(sizeof(exprlist));
        l->expr = $1;
        l->next = NULL;
        $$ = l;
    }
    | indexed_list PUNCTUATION_COMMA indexedelem {
        exprlist* l = malloc(sizeof(exprlist));
        l->expr = $3;
        l->next = $1;
        $$ = l;
    }
;

indexedelem: PUNCTUATION_LEFT_BRACE expr PUNCTUATION_COLON expr PUNCTUATION_RIGHT_BRACE {
    expr* e = $4;         // value
    e->index = $2;        // key
    $$ = e;
}



funcdef: funcdecl funcbody {
    // Store the number of local variables in the function symbol
    $1->total_locals = $2; 
    // Exit the scopespace for function locals
    exitscopespace(); 

    // Patch all return jumps to point here (function end)
    patch_return_jumps(nextquad());
    //emit quad
    emit(funcend, newexpr_programfunc($1), NULL, NULL, 0, yylineno);
    patchlabel($1->jump_quad, nextquad());

    int offset = pop_and_top(scopeoffsetstack);
    restorecurrscopeoffset(offset); //restore scope

    parsing_funcdef--;
    deactivate_scope(symbol_table, current_scope);
    current_scope--;
    // Return an expr* representing the function
    expr* e = newexpr(programfunc_e);
    e->sym = $1;
    $$ = e;
};

funcdecl: funcdecl_head idlist PUNCTUATION_RIGHT_PARENTHESIS {
    enterscopespace();           
    resetfunctionlocalsoffset(); 
    $$ = $1;
};

funcdecl_head: KEYWORD_FUNCTION IDENT PUNCTUATION_LEFT_PARENTHESIS {
    // Check for redeclaration in the current scope
    Symbol *existing = lookup_symbol(symbol_table, $2, current_scope, 1);
    if (existing) {
        fprintf(stderr, "Error: Function '%s' already declared in scope %d at line %d.\n", $2, current_scope, yylineno);
        exit(1);
    }
     // Prevent shadowing of library functions in the global scope
    Symbol *global_conflict = lookup_symbol(symbol_table, $2, 0, 0);
    if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", $2, yylineno);
        exit(1);
    }

    Symbol *funcsym = insert_symbol(symbol_table, $2, USER_FUNCTION, yylineno, current_scope);

    // Emit initial jump to skip function body
    int jump_quad = nextquad();
    emit(jump, NULL, NULL, NULL, 0, yylineno);
   // possible change add_return_jump(jump_quad);
    funcsym->jump_quad = jump_quad;
    // Emit funcstart quad
    emit(funcstart, newexpr_programfunc(funcsym), NULL, NULL, 0, yylineno);
    // Save the current scope offset and enter scopespace for formals
    push(scopeoffsetstack, currscopeoffset());
    enterscopespace();           // Είσοδος στα formals
    resetformalargsoffset();     // Ξεκινά τα formals από offset 0
    // Enter new scope for function body
    current_scope++;
    parsing_funcdef++;

    $$ = funcsym;
};

funcbody: block {
    // Return the number of local variables (offset) and exit the scopespace
    $$ = currscopeoffset(); 
    exitscopespace();       
};

function_block: PUNCTUATION_LEFT_BRACE stmt_list PUNCTUATION_RIGHT_BRACE {
    $$ = $2;
 //   printf("DEBUG: Function body parsed.\n");
}
| PUNCTUATION_LEFT_BRACE PUNCTUATION_RIGHT_BRACE {
     // Handle empty function body or block
    if (parsing_anon_funcdef) {
 //       printf("DEBUG: Empty anonymous function body at line %d.\n", yylineno);
    } else {
  //      printf("DEBUG: Empty named function body or standalone block at line %d.\n", yylineno);
    }
    $$ = NULL;
};

anonfuncdef: anonfuncdecl anonfuncbody {
    // Patch all return jumps to point here (end of anonymous function)
    patch_return_jumps(nextquad());
    // Emit funcend quad for the anonymous function
    emit(funcend, newexpr_programfunc($1), NULL, NULL, 0, yylineno);
    printf("DEBUG: Patching jump at quad %d to label %d (nextquad)\n", $1->jump_quad, nextquad());
    patchlabel($1->jump_quad, nextquad());

    parsing_anon_funcdef--;
    deactivate_scope(symbol_table, current_scope);
    current_scope--;

    
    expr* e = newexpr(programfunc_e);
    e->sym = $1;
    $$ = e;
};

anonfuncdecl: anonfuncdecl_head idlist PUNCTUATION_RIGHT_PARENTHESIS {
    $$ = $1;
}
anonfuncdecl_head: KEYWORD_FUNCTION PUNCTUATION_LEFT_PARENTHESIS {
    parsing_anon_funcdef++; // INCREMENT for nested anonymous functions
    // Generate a unique name for the anonymous function
    static int anonymous_function_counter = 0;
    char *anon_name = malloc(64);
    snprintf(anon_name, 64, "anonymous_function_%d", ++anonymous_function_counter);
     // Prevent shadowing of library functions
    Symbol *global_conflict = lookup_symbol(symbol_table, anon_name, 0, 0);
    if (global_conflict && global_conflict->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", anon_name, yylineno);
        exit(1);
    }

    // Emit initial jump to skip the function body
    int jump_quad = nextquad();
    printf("DEBUG: Emitting initial jump at quad %d\n", jump_quad);
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    // add_return_jump(jump_quad);

    insert_symbol(symbol_table, anon_name, USER_FUNCTION, yylineno, current_scope);

    // emit funcstart
    emit(funcstart, newexpr_programfunc_name(anon_name), NULL, NULL, 0, yylineno);

    current_scope++;

    $$ = malloc(sizeof(Symbol));
    $$->type = USER_FUNCTION;
    $$->name = anon_name;
    $$->line = yylineno;
    $$->scope = current_scope - 1;
    $$->jump_quad = jump_quad;
};

anonfuncbody: function_block {
    $$ = $1;
 //   printf("DEBUG: Anonymous function body parsed.\n");
};




block: block_start stmt_list block_end {
  //  printf("DEBUG: Parsing block with statements.\n");
    $$ = $2; // Pass the value of stmt_list
}
| block_start block_end {
    if (parsing_anon_funcdef) {
   //     printf("DEBUG: Empty anonymous function body at line %d.\n", yylineno);
    } else {
 //       printf("DEBUG: Empty standalone block at line %d.\n", yylineno);
    }
    $$ = NULL; // Handle empty block
};

block_start: PUNCTUATION_LEFT_BRACE {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        current_scope++; // Increment scope for standalone blocks
  //      printf("DEBUG: Entering block scope %d at line %d.\n", current_scope, yylineno);
    } else {
       // printf("DEBUG: Skipping scope increment for function body at line %d.\n", yylineno);
    }
};

block_end: PUNCTUATION_RIGHT_BRACE {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
   //     printf("DEBUG: Closing standalone block at line %d.\n", yylineno);
        current_scope--; // <-- Make sure this is here!
    } else if (parsing_anon_funcdef) {
  //      printf("DEBUG: Closing anonymous function body at line %d.\n", yylineno);
    } else if (parsing_funcdef) {
  //      printf("DEBUG: Closing named function body at line %d.\n", yylineno);
    }
};





const: INTCONST        { $$ = newexpr_constnum($1); }
     | REALCONST       { $$ = newexpr_constnum($1); }
     | STRINGCONST     { $$ = newexpr_conststring($1); }
     | KEYWORD_NIL     { $$ = newexpr_nil(); }
     | KEYWORD_TRUE    { $$ = newexpr_constbool(1); }
     | KEYWORD_FALSE   { $$ = newexpr_constbool(0); }
;

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
      //      printf("DEBUG: Inserting formal argument '%s' into symbol table at line %d in scope %d.\n", $3, yylineno, argument_scope);
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
       //     printf("DEBUG: Inserting formal argument '%s' into symbol table at line %d in scope %d.\n", $1, yylineno, argument_scope);
        };

ifstmt
    : ifprefix stmt %prec LOWER_THAN_ELSE {
        patchlabel($1, nextquad());
    }
    | ifprefix stmt elseprefix stmt {
        patchlabel($1, $3 + 1);      // Patch IF_EQ to else-branch
        patchlabel($3, nextquad());  // Patch jump after then-branch to after else-branch
    }
;

ifprefix
    : KEYWORD_IF PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS {
        expr* cond = $3;
        if (
            cond->type == constbool_e ||
            cond->type == var_e
        ) {
            emit(if_eq, cond, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
            $$ = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno); // Always emit jump after IF_EQ
        } else if (
            cond->type == and_e ||
            cond->type == or_e ||
            cond->type == relop_e ||
            cond->type == not_e ||
            cond->type == boolexpr_e
        ) {
            expr* boolified = to_bool(cond);
            backpatch(boolified->truelist, nextquad() + 1);
            $$ = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno); // Always emit jump after IF_EQ
        } else {
            emit(if_eq, cond, newexpr_constbool(1), NULL, nextquad() + 2, yylineno);
            $$ = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno); // Always emit jump after IF_EQ
        }
    }
;

elseprefix
    : KEYWORD_ELSE {
        $$ = nextquad();
         emit(jump, NULL, NULL, NULL, 0, yylineno);
    }
;







loopstmt:
    /* empty */ { 
        ++loop_depth; 
        fprintf(stderr, "DEBUG: loopstmt - entering loop, depth now %d\n", loop_depth);
    }
    stmt { 
        --loop_depth; 
        fprintf(stderr, "DEBUG: loopstmt - exiting loop, depth now %d\n", loop_depth);
        $$ = $2; 
    }
;


whilestart:
    /* empty */ { 
        $$ = nextquad();
        fprintf(stderr, "DEBUG: whilestart - nextquad is %u\n", $$+1);
    }
;

whilecond:
    PUNCTUATION_LEFT_PARENTHESIS expr PUNCTUATION_RIGHT_PARENTHESIS {
        expr* cond = to_bool($2);
        fprintf(stderr, "DEBUG: whilecond - after to_bool, truelist head is %d, falselist head is %d\n",
            cond->truelist ? cond->truelist->quad : -1,
            cond->falselist ? cond->falselist->quad : -1);

        backpatch(cond->truelist, nextquad());

        fprintf(stderr, "DEBUG: whilecond - after backpatch truelist to %u\n", nextquad()+1);

        $$ = cond; // Just pass the expr* with truelist/falselist
    }
;

whilestmt:
    KEYWORD_WHILE whilestart whilecond {
        // Entering a new loop: push empty lists
        breaklist_stack[++breaklist_stack_top] = NULL;
        continuelist_stack[++continuelist_stack_top] = NULL;
    }
    loopstmt {
        emit(jump, NULL, NULL, NULL, $2, yylineno);
        patchlist($3->falselist, nextquad());

        // Patch all breaks and continues for this loop
        patchlist(breaklist_stack[breaklist_stack_top], nextquad());
        patchlist(continuelist_stack[continuelist_stack_top], $2);

        // Pop the lists
        breaklist_stack_top--;
        continuelist_stack_top--;

        $$ = NULL;
    }
;



N1: /* empty */ {
    $$ = nextquad();
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    // fprintf(stderr, "DEBUG: N1 marker (no jump) at quad %u\n", curr_quad);
};
N2: /* empty */ {
    $$ = nextquad();
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    // fprintf(stderr, "DEBUG: N2 marker emits jump at quad %u\n", curr_quad);
};
N3: /* empty */ {
    $$ = nextquad();
    printf("DEBUG: N3 is quad %d\n", $$);
    emit(jump, NULL, NULL, NULL, 0, yylineno);
};


M: /* empty */ {
    $$ = nextquad();
};


forprefix:
    KEYWORD_FOR PUNCTUATION_LEFT_PARENTHESIS
    elist PUNCTUATION_SEMICOLON
    M expr PUNCTUATION_SEMICOLON
    {
        forprefix_t fp;
        fp.test = $5; // $5 is M
        fp.enter = nextquad();
        emit(if_eq, $6, newexpr_constbool(1), NULL, 0, yylineno);
        $$ = fp;
    }
;

forstmt:
    forprefix N1 elist PUNCTUATION_RIGHT_PARENTHESIS N2 loopstmt N3
    {
        // Entering a new loop: push empty lists
        breaklist_stack[++breaklist_stack_top] = NULL;
        continuelist_stack[++continuelist_stack_top] = NULL;

        // ... your patchlabel logic ...
        patchlabel($1.enter, $5 + 1);      // true jump: if_eq true -> body
        patchlabel($2, nextquad());        // false jump: if_eq false -> exit
        patchlabel($5, $1.test);           // loop jump: after increment -> test
        patchlabel($7, $2 + 1);            // closure jump: after body -> increment

        // Patch all breaks and continues for this loop
        patchlist(breaklist_stack[breaklist_stack_top], nextquad());
        patchlist(continuelist_stack[continuelist_stack_top], $5 + 1);

        // Pop the lists
        breaklist_stack_top--;
        continuelist_stack_top--;
    }
;






























/*      testing simpler format for now , error with m vlaue returning loop
forprefix:
    KEYWORD_FOR PUNCTUATION_LEFT_PARENTHESIS
    {
        loop_depth++;
    }
    elist PUNCTUATION_SEMICOLON
    M expr PUNCTUATION_SEMICOLON
    {
        forprefix_t fp;
        fp.test = $6; // $6 is M, which is nextquad() before the test


        
        // Check if the test is a simple variable or constant
        if ($7->type == var_e || $7->type == constbool_e || $7->type == constnum_e) {
            // Minimal quads: just emit if_eq and jump
            fp.enter = nextquad();
            emit(if_eq, $7, newexpr_constbool(1), NULL, 0, yylineno); // if_eq
           // fp.jump_out_quad = nextquad();
          //  emit(jump, NULL, NULL, NULL, 0, yylineno); // unconditional jump (exit)
        } else {
            // Full canonical translation (as you have now)
            expr* cond = to_bool($7);
            expr* temp = newexpr(var_e);
            temp->sym = newtemp();

            unsigned quad_true = nextquad();
            emit(assign, temp, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, quad_true + 2 + 1, yylineno);

            unsigned quad_false = nextquad();
            emit(assign, temp, newexpr_constbool(0), NULL, 0, yylineno);

            backpatch(cond->truelist, quad_true);
            backpatch(cond->falselist, quad_false);

            fp.enter = nextquad();
            emit(if_eq, temp, newexpr_constbool(1), NULL, 0, yylineno);
            // fp.jump_out_quad = nextquad();
           // emit(jump, NULL, NULL, NULL, 0, yylineno);
        }

        $$ = fp;
    }
;

forstmt:
    forprefix N1 elist PUNCTUATION_RIGHT_PARENTHESIS N2 stmt N3
    {
        // $2 = N1 (after forprefix, before increment elist)      // N1 marker: jump after test, to exit
        // $5 = N2 (after increment elist, before stmt)            // N2 marker: jump after increment, to test
        // $7 = N3 (after stmt)                                    // N3 marker: jump after body, to increment
        // $1.test = test quad (from forprefix)
        // $1.enter = if_eq quad (from forprefix)
        // $1.jump_out_quad = jump after if_eq (from forprefix)
        fprintf(stderr,
        "DEBUG: $1.enter=%u (if_eq), $1.test=%u (test), $2 (N1)=%u, $5 (N2)=%u, $7 (N3)=%u, nextquad()=%u\n",
        $1.enter, $1.test, $2, $5, $7, nextquad());

        // 1. Patch the if_eq (test) to jump to the body if true
        patchlabel($1.enter, $5 + 1 );      // if_eq true -> body (first quad of stmt)
        // 2. Patch the jump after test (N1) to exit after the loop
        patchlabel($2, nextquad() );        // if_eq false -> exit (after loop)
        // 3. Patch the jump after increment (N2) to the test
        patchlabel($5, $1.test);            // after increment -> test (first quad of test)
        // 4. Patch the jump after body (N3) to the increment
        patchlabel($7, $5 + 1 );  // after body -> increment (first quad of increment)

        
        loop_depth--;
        $$ = 0;
    }
;

*/

returnstmt: return_no_expr
          | return_with_expr;

return_no_expr: KEYWORD_RETURN %prec KEYWORD_RETURN {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        fprintf(stderr, "Error: Use of 'return' while not in a function at line %d.\n", yylineno);
        exit(1);
    }
    emit(return_, NULL, NULL, NULL, 0, yylineno);
    emit(jump, NULL, NULL, NULL, 0, yylineno); 
    add_return_jump(nextquad() - 1);           
};

return_with_expr: KEYWORD_RETURN expr {
    if (!parsing_funcdef && !parsing_anon_funcdef) {
        fprintf(stderr, "Error: Use of 'return' while not in a function at line %d.\n", yylineno);
        exit(1);
    }
    emit(return_, $2, NULL, NULL, 0, yylineno);
    emit(jump, NULL, NULL, NULL, 0, yylineno); 
    add_return_jump(nextquad() - 1);           
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

    // Print the quads after parsing
    print_quads("quads.txt");

    // Destroy the symbol table to free memory
    destroy_symbol_table(symbol_table);

    if (yyin) {
        fclose(yyin);
    }

    return 0;
}