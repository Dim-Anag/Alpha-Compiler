#ifndef AVM_H
#define AVM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "avm_instruction.h" 

#define AVM_STACKSIZE 4096
#define AVM_STACKENV_SIZE 4
#define AVM_NUMACTUALS_OFFSET 4
#define AVM_SAVEDPC_OFFSET    3
#define AVM_SAVEDTOP_OFFSET   2
#define AVM_SAVEDTOPSP_OFFSET 1

/* Memory cell types    */
typedef enum {
    number_m, 
    string_m, 
    bool_m, 
    table_m, 
    userfunc_m, 
    libfunc_m, 
    nil_m, 
    undef_m
} avm_memcell_t;


typedef struct avm_table avm_table;

/* Memory cell structure    */
typedef struct avm_memcell {
    avm_memcell_t type;
    union {
        double      numVal;
        char*       strVal;
        unsigned    boolVal;
        struct avm_table* tableVal;
        unsigned    funcVal;
        char*       libfuncVal;
    } data;
} avm_memcell;


extern avm_memcell ax, bx, cx, retval;


extern avm_memcell stack[AVM_STACKSIZE];
extern unsigned top, topsp, pc, executionFinished, codeSize;
extern instruction* code;


typedef void (*execute_func_t)(instruction*);
typedef void (*library_func_t)(void);


void avm_initialize(void);
void execute_cycle(void);
avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg);
void avm_memcellclear(avm_memcell* m);
void avm_assign(avm_memcell* lv, avm_memcell* rv);



#endif