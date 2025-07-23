#ifndef VARTEMPMAP_H
#define VARTEMPMAP_H

#include "expr.h"
// Initializes the variable-to-temp map (clears all entries)
void vartempmap_init(void);
// Clears the variable-to-temp map and frees memory for variable names
void vartempmap_clear(void);
// Returns the temp expr for a variable name, or creates one if it doesn't exist
struct expr* get_or_create_temp(const char* varname);
// Returns 1 if the expr is a temp (name starts with '^' or '_'), 0 otherwise
int istempexpr(struct expr* e);
#endif