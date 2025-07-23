#include "avm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "avm_const_tables.h"
#include "avm_instruction.h"
#include "avm_tables.h"
#define GLOBALS_SIZE (max_global_offset + 1)

int max_global_offset = -1;
unsigned totalActuals = 0;
extern double* numConsts;
extern unsigned totalNumConsts;
extern char** stringConsts;
extern unsigned totalStringConsts;
extern char** namedLibfuncs;
extern unsigned totalNamedLibfuncs;



double consts_getnumber(unsigned index) {
    assert(index < totalNumConsts);
    return numConsts[index];
}

char* consts_getstring(unsigned index) {
    assert(index < totalStringConsts);
    return stringConsts[index];
}

char* libfuncs_getused(unsigned index) {
    assert(index < totalNamedLibfuncs);
    return namedLibfuncs[index];
}


/* Type-to-string conversion functions  */
char* number_tostring(avm_memcell* m) {
    char* buf = (char*)malloc(32);
    snprintf(buf, 32, "%.3f", m->data.numVal);
    return buf;
}

char* string_tostring(avm_memcell* m) {
    return strdup(m->data.strVal ? m->data.strVal : "");
}

char* bool_tostring(avm_memcell* m) {
    return strdup(m->data.boolVal ? "true" : "false");
}

char* nil_tostring(avm_memcell* m) {
    return strdup("nil");
}

char* undef_tostring(avm_memcell* m) {
    return strdup("undefined");
}

char* userfunc_tostring(avm_memcell* m) {
    char* buf = (char*)malloc(64);
    snprintf(buf, 64, "user function @%u", m->data.funcVal);
    return buf;
}

char* libfunc_tostring(avm_memcell* m) {
    char* buf = (char*)malloc(64);
    snprintf(buf, 64, "library function %s", m->data.libfuncVal);
    return buf;
}


char* table_tostring(avm_memcell* m) {
    return strdup("[table]");
}

/* Dispatch table for tostring  */
typedef char* (*tostring_func_t)(avm_memcell*);
tostring_func_t tostringFuncs[] = {
    number_tostring,
    string_tostring,
    bool_tostring,
    table_tostring,
    userfunc_tostring,
    libfunc_tostring,
    nil_tostring,
    undef_tostring
};

char* avm_tostring(avm_memcell* m) {
    if (m->type < 0 || m->type > undef_m) return strdup("unknown");
    return (*tostringFuncs[m->type])(m);
}


unsigned char number_tobool(avm_memcell* m) { return m->data.numVal != 0; }
unsigned char string_tobool(avm_memcell* m) { return m->data.strVal[0] != 0; }
unsigned char bool_tobool(avm_memcell* m)   { return m->data.boolVal; }
unsigned char table_tobool(avm_memcell* m)  { return 1; }
unsigned char userfunc_tobool(avm_memcell* m) { return 1; }
unsigned char libfunc_tobool(avm_memcell* m)  { return 1; }
unsigned char nil_tobool(avm_memcell* m)      { return 0; }
unsigned char undef_tobool(avm_memcell* m)    { return 0; }

typedef unsigned char (*tobool_func_t)(avm_memcell*);
tobool_func_t toboolFuncs[] = {
    number_tobool,
    string_tobool,
    bool_tobool,
    table_tobool,
    userfunc_tobool,
    libfunc_tobool,
    nil_tobool,
    undef_tobool
};

unsigned char avm_tobool(avm_memcell* m) {
    if (m->type < 0 || m->type > undef_m) return 0;
    return (*toboolFuncs[m->type])(m);
}


void avm_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "AVM ERROR: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    executionFinished = 1;
}

void avm_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "AVM WARNING: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}


extern userfunc* userFuncs;
extern unsigned totalUserFuncs;


userfunc* userfuncs_getfunc(unsigned index) {
    if (index < totalUserFuncs)
        return &userFuncs[index];
    else
        return NULL;
}

/* Converts a vmarg to an avm_memcell pointer or fills 'reg' with its value */
avm_memcell* avm_translate_operand(vmarg* arg, avm_memcell* reg) {
//    printf("DEBUG: avm_translate_operand: type=%d, val=%u\n", arg->type, arg->val);
    switch (arg->type) {
        case global_a:
        case temp_a: 
          //  printf("DEBUG: global/temp offset=%u, type in stack=%d\n", arg->val, stack[AVM_STACKSIZE - GLOBALS_SIZE + arg->val].type);
            return &stack[AVM_STACKSIZE - GLOBALS_SIZE + arg->val];
        case local_a:
            return &stack[topsp - arg->val];
        case formal_a:
            return &stack[topsp + AVM_STACKENV_SIZE + 1 + arg->val];
        case retval_a:
            return &retval;
        case number_a:
            reg->type = number_m;
            reg->data.numVal = consts_getnumber(arg->val);
         //   printf("DEBUG: avm_translate_operand: type=%d, val=%u, numConst=%f\n", arg->type, arg->val, consts_getnumber(arg->val));
            return reg;
        case string_a:
            avm_memcellclear(reg);
            reg->type = string_m;
            reg->data.strVal = strdup(consts_getstring(arg->val));
            return reg;
        case bool_a:
            reg->type = bool_m;
            reg->data.boolVal = arg->val;
            //    printf("DEBUG: avm_translate_operand: bool, val=%u\n", arg->val);
            return reg;
        case nil_a:
            reg->type = nil_m;
         //   printf("DEBUG: avm_translate_operand: nil\n");
            return reg;
        case userfunc_a:
            reg->type = userfunc_m;
            reg->data.funcVal = userfuncs_getfunc(arg->val)->address;
            return reg;
        case libfunc_a:
          //  printf("DEBUG: avm_translate_operand: libfunc, val=%u\n", arg->val);
            assert(arg->val < totalLibFuncs); 
            reg->type = libfunc_m;
            reg->data.libfuncVal = libFuncs[arg->val];
  //  printf("DEBUG: avm_translate_operand: libfunc, val=%u, name=%s\n", arg->val, reg->data.libfuncVal);
            return reg;
        default:
            assert(0);
            return NULL;
    }
}


/* assigns the value of rv to lv, clearing lv first */
void avm_assign(avm_memcell* lv, avm_memcell* rv) {
 //   printf("DEBUG: avm_assign: lv=%p, rv=%p, lv->type=%d, rv->type=%d\n", (void*)lv, (void*)rv, lv->type, rv->type);
    if (lv == rv) return;

    if (lv->type == table_m && rv->type == table_m && lv->data.tableVal == rv->data.tableVal)
        return;

    if (rv->type == undef_m)
        avm_warning("assigning from 'undef' content!");


   // printf("DEBUG: avm_assign: before avm_memcellclear\n");
    avm_memcellclear(lv);
   // printf("DEBUG: avm_assign: after avm_memcellclear\n");


    switch (rv->type) {
        case number_m:
       //  printf("DEBUG: avm_assign: assigning number %f\n", rv->data.numVal);
            lv->type = number_m;
            lv->data.numVal = rv->data.numVal;
            break;
        case string_m:
       //  printf("DEBUG: avm_assign: assigning string '%s'\n", rv->data.strVal);
            lv->type = string_m;
            lv->data.strVal = strdup(rv->data.strVal ? rv->data.strVal : "");
            break;
        case bool_m:
      //  printf("DEBUG: avm_assign: assigning bool %d\n", rv->data.boolVal);
            lv->type = bool_m;
            lv->data.boolVal = rv->data.boolVal;
            break;
        case nil_m:
       // printf("DEBUG: avm_assign: assigning nil\n");
            lv->type = nil_m;
            break;
        case table_m:
            lv->type = table_m;
            lv->data.tableVal = rv->data.tableVal;
            avm_tableincrefcounter(lv->data.tableVal);
          //  printf("DEBUG: assigned table, address=%p\n", (void*)lv->data.tableVal);
            break;
        case userfunc_m:
            lv->type = userfunc_m;
            lv->data.funcVal = rv->data.funcVal;
            break;
        case libfunc_m:
            lv->type = libfunc_m;
            lv->data.libfuncVal = rv->data.libfuncVal;
            break;
        case undef_m:
            lv->type = undef_m;
            break;
        default:
             printf("DEBUG: avm_assign: unknown type %d\n", rv->type);
            break;
    }
  //  printf("DEBUG: objectcopy() retval.type=%d\n", retval.type);
  //  printf("DEBUG: avm_assign: after assign, lv->type=%d\n", lv->type);
}


void memclear_string(avm_memcell* m) {
    if (m->data.strVal)
        free(m->data.strVal);
}

void memclear_table(avm_memcell* m) {
    assert(m->data.tableVal);
    avm_tabledecrefcounter(m->data.tableVal);
}


typedef void (*memclear_func_t)(avm_memcell*);
memclear_func_t memclearFuncs[] = {
    0,                
    memclear_string,  
    0,                
    memclear_table,   
    0,               
    0,                
    0,                
    0                 
};

void avm_memcellclear(avm_memcell* m) {
  //  printf("DEBUG: avm_memcellclear: m=%p, type=%d\n", (void*)m, m->type);
    if (m->type < 0 || m->type > undef_m) {
        printf("ERROR: avm_memcellclear: invalid type %d\n", m->type);
        return;
    }
    if (m->type != undef_m) {
        memclear_func_t f = memclearFuncs[m->type];
        if (f)
            (*f)(m);
        m->type = undef_m;
    }
}

/* Helper to get an environment value from the stack    */
unsigned avm_get_envvalue(unsigned i) {
    assert(stack[i].type == number_m);
    unsigned val = (unsigned)stack[i].data.numVal;
    assert(stack[i].data.numVal == ((double)val));
    return val;
}

/* Save the environment before a function call  */
void avm_push_envvalue(unsigned val) {
    assert(top > 0); // Για να μην βγεις εκτός ορίων
    --top;
    stack[top].type = number_m;
    stack[top].data.numVal = val;
}

/* Save the environment (activation record) before a call   */
void avm_callsaveenvironment(void) {
  
    avm_push_envvalue(totalActuals);   /* [top] = πλήθος actuals    */
    avm_push_envvalue(pc + 1);         /* [top+1] = επόμενο pc  */
    avm_push_envvalue(topsp);          /* [top+2] = topsp   */
    avm_push_envvalue(top);            /* [top+3] = top */
    topsp = top - 4;                        /* push των env values  */
   // totalActuals = 0;
}

unsigned avm_totalactuals(void) {
    return totalActuals;
}

avm_memcell* avm_getactual(unsigned i) {
    return &stack[top + 4 + i];
}