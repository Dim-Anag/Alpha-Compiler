#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "temp.h"
// Counter for generating unique temporary variable names
static unsigned temp_counter = 0;
// Creates and returns a new temporary Symbol (not inserted in the symbol table) (to eixa kai me mperdeue na bgazei 
// sto output tou parser ta ^0 ktl)
Symbol* newtemp(void) {
    char name[32];
    snprintf(name, sizeof(name), "^%u", temp_counter++);
    Symbol* s = malloc(sizeof(Symbol));
    s->name = strdup(name); // Set the unique name
    s->type = VARIABLE;     // Mark as variable type
    s->scope = 0; 
    s->line = 0;
    return s;
}