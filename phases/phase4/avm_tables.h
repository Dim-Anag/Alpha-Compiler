#ifndef AVM_TABLES_H
#define AVM_TABLES_H

#include "avm.h"

typedef struct avm_table avm_table;
typedef struct avm_memcell avm_memcell;

/* Allocates and returns a new AVM table. */
avm_table* avm_tablenew(void);

/* Increments the reference counter of the table. */
void avm_tableincrefcounter(avm_table* t);

/* Decrements the reference counter of the table and frees it if needed. */
void avm_tabledecrefcounter(avm_table* t);

/* Returns a pointer to the value associated with the given key in the table. */
avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key);

/* Sets the value for the given key in the table. */
void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* value);

/* Returns the total number of members in the table. */
unsigned avm_table_totalmembers(avm_table* t);

/* Fills result_array with all keys of the table and returns their count. */
unsigned avm_table_keys(avm_table* t, avm_memcell* result_array);

#endif