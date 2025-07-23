#ifndef AVM_LIBFUNCS_H
#define AVM_LIBFUNCS_H

#include "avm.h"

extern unsigned totalNamedLibfuncs;
extern char** namedLibfuncs;

struct avm_table;
typedef struct avm_table avm_table;

/* Table management functions */
avm_table* avm_tablenew(void); /* Allocates and returns a new AVM table. */
void avm_tableincrefcounter(avm_table* t); /* Increments table reference counter. */
unsigned avm_table_totalmembers(avm_table* t); /* Returns total number of table members. */
unsigned avm_table_keys(avm_table* t, avm_memcell* result_array); /* Fills result_array with all table keys. */
avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key); /* Gets value for a key in the table. */
void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* value); /* Sets value for a key in the table. */

/* Argument and stack helpers */
unsigned avm_totalactuals(void); /* Returns total number of actual arguments. */
avm_memcell* avm_getactual(unsigned i); /* Returns pointer to the i-th actual argument. */
char* avm_tostring(avm_memcell* m); /* Converts an avm_memcell to a string. */
void avm_memcellclear(avm_memcell* m); /* Clears an avm_memcell. */
void avm_assign(avm_memcell* lv, avm_memcell* rv); /* Assigns rv to lv. */
unsigned avm_get_envvalue(unsigned i); /* Gets environment value from stack. */

/* Built-in library functions */
void libfunc_print(void);
void libfunc_input(void);
void libfunc_typeof(void);
void libfunc_totalarguments(void);
void libfunc_argument(void);
void libfunc_objectmemberkeys(void);
void libfunc_objecttotalmembers(void);
void libfunc_objectcopy(void);
void libfunc_strtonum(void);
void libfunc_sqrt(void);
void libfunc_cos(void);
void libfunc_sin(void);

typedef void (*library_func_t)(void);

/* Structure for mapping library function names to their addresses */
typedef struct libfunc_entry {
    const char* id;
    library_func_t addr;
} libfunc_entry;

/* Calls a library function by its name. */
void avm_calllibfunc(char* id);

#endif