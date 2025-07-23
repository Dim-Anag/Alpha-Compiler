#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "temp.h"

static unsigned temp_counter = 0;
extern int yylineno;
extern int temp_var_offset;




Symbol* newtemp(SymbolTable* sym_table, int scope) {
    char name[32];
    snprintf(name, sizeof(name), "^%u", temp_counter++);
    Symbol* s = insert_symbol(sym_table, name, TEMPVAR, yylineno, scope);

    
   // printf("DEBUG: newtemp created name=%s, type=%d, addr=%p, space=%d, offset=%d\n",
   //     s->name, s->type, (void*)s, s->space, s->offset);
    return s;
}