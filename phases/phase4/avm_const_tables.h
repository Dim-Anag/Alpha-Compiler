#ifndef AVM_CONST_TABLES_H
#define AVM_CONST_TABLES_H

#include "avm_instruction.h"

/* Arrays and counters for constants and functions. */
extern double* numConsts;
extern unsigned totalNumConsts;

extern char** stringConsts;
extern unsigned totalStringConsts;

extern userfunc* userFuncs;
extern unsigned totalUserFuncs;

extern char** libFuncs;
extern unsigned totalLibFuncs;

/* Adds a new number constant if not present, returns its index. */
unsigned consts_newnumber(double n);

/* Adds a new string constant if not present, returns its index. */
unsigned consts_newstring(const char* s);

/* Adds a new user function if not present, returns its index. */
unsigned userfuncs_newfunc(userfunc f);

/* Adds a new library function if not present, returns its index. */
unsigned libfuncs_newused(const char* s);

/* Initializes the constant tables. */
void init_const_tables(void);

/* Frees all memory used by the constant tables. */
void free_const_tables(void);

unsigned lookup_or_insert_libfunc(const char* name);

#endif /* AVM_CONST_TABLES_H */