#ifndef AVM_UTILS_H
#define AVM_UTILS_H

#include "avm.h"
#include "avm_tables.h"
#include "avm_instruction.h"

/* Returns the number constant at the given index. */
double consts_getnumber(unsigned index);

/* Returns the string constant at the given index. */
char* consts_getstring(unsigned index);

/* Returns the name of the used library function at the given index. */
char* libfuncs_getused(unsigned index);

/* Converts an avm_memcell to a string representation. */
char* avm_tostring(avm_memcell* m);

/* Converts an avm_memcell to a boolean value. */
unsigned char avm_tobool(avm_memcell* m);

/* Prints an error message and stops execution. */
void avm_error(const char* format, ...);

/* Prints a warning message. */
void avm_warning(const char* format, ...);

extern userfunc* userFuncs;
extern unsigned totalUserFuncs;

/* Returns a pointer to the user function at the given index. */
userfunc* userfuncs_getfunc(unsigned index);

/* Converts a vmarg to an avm_memcell pointer or fills 'reg' with its value. */
avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg);

/* Assigns the value of rv to lv. */
void avm_assign(avm_memcell* lv, avm_memcell* rv);

/* Clears the contents of an avm_memcell. */
void avm_memcellclear(avm_memcell* m);

/* Returns the environment value from the stack at index i. */
unsigned avm_get_envvalue(unsigned i);

/* Pushes an environment value onto the stack. */
void avm_push_envvalue(unsigned val);

/* Saves the current environment before a function call. */
void avm_callsaveenvironment(void);

/* Returns the total number of actual arguments for the current function. */
unsigned avm_totalactuals(void);

/* Returns a pointer to the i-th actual argument for the current function. */
avm_memcell* avm_getactual(unsigned i);

void print_stack(unsigned start, unsigned end);

#endif