#ifndef AVM_INSTRUCTION_H
#define AVM_INSTRUCTION_H

extern unsigned totalNamedLibfuncs;
extern char** namedLibfuncs;

/* Virtual machine opcodes for all supported instructions. */
typedef enum {
    assign_v,        /* 0 */
    add_v,           /* 1 */
    sub_v,           /* 2 */
    mul_v,           /* 3 */
    div_v,           /* 4 */
    mod_v,           /* 5 */
    uminus_v,        /* 6 */
    and_v,           /* 7 */
    or_v,            /* 8 */
    not_v,           /* 9 */
    jeq_v,           /* 10 */
    jne_v,           /* 11 */
    jle_v,           /* 12 */
    jge_v,           /* 13 */
    jlt_v,           /* 14 */
    jgt_v,           /* 15 */
    jump_v,          /* 16 */
    call_v,          /* 17 */
    pusharg_v,       /* 18 */
    funcenter_v,     /* 19 */
    funcexit_v,      /* 20 */
    newtable_v,      /* 21 */
    tablegetelem_v,  /* 22 */
    tablesetelem_v,  /* 23 */
    nop_v,           /* 24 */
    getretval_v      /* 25 */
} vmopcode;

/* Types of operands for instructions. */
typedef enum {
    label_a = 0,
    global_a = 1,
    formal_a = 2,
    local_a = 3,
    number_a = 4,
    string_a = 5,
    bool_a = 6,
    nil_a = 7,
    userfunc_a = 8,
    libfunc_a = 9,
    retval_a = 10,
    temp_a = 11
} vmarg_t;

/* Structure representing an operand (argument) of an instruction. */
typedef struct {
    vmarg_t type;
    unsigned val;
} vmarg;

/* Structure representing a VM instruction. */
typedef struct {
    vmopcode opcode;
    vmarg result;
    vmarg arg1;
    vmarg arg2;
    unsigned srcLine;
} instruction;

/* Structure representing a user-defined function. */
typedef struct {
    unsigned address;
    unsigned localSize;
    char* id;
} userfunc;

#endif /* AVM_INSTRUCTION_H */