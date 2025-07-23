#include "avm_codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "quads.h"
#include "avm_instruction.h"
#include "avm_const_tables.h"
#include "expr.h"


/* Dynamic array for instructions   */
instruction* instructions = NULL;
static unsigned currInstruction = 0;
static unsigned totalInstructions = 0;
static unsigned functionLocalOffset = 0;
static unsigned formalArgOffset = 0;
extern int temp_var_offset;
#define INITIAL_INSTR_SIZE 256
unsigned libfuncs_newused(const char* s);
// Incomplete jumps structure
typedef struct incomplete_jump {
    unsigned instrNo;
    unsigned iaddress;
    struct incomplete_jump* next;
} incomplete_jump;

typedef void (*generator_func_t)(quad*);


static incomplete_jump* ij_head = NULL;



void generate_ASSIGN(quad* q);
void generate_ADD(quad* q);
void generate_SUB(quad* q);
void generate_MUL(quad* q);
void generate_DIV(quad* q);
void generate_MOD(quad* q);
void generate_UMINUS(quad* q);
void generate_AND(quad* q);
void generate_OR(quad* q);
void generate_NOT(quad* q);
void generate_IF_EQ(quad* q);
void generate_IF_NOTEQ(quad* q);
void generate_IF_LESS(quad* q);
void generate_IF_LESSEQ(quad* q);
void generate_IF_GREATER(quad* q);
void generate_IF_GREATEREQ(quad* q);
void generate_JUMP(quad* q);
void generate_CALL(quad* q);
void generate_PARAM(quad* q);
void generate_GETRETVAL(quad* q);
void generate_FUNCSTART(quad* q);
void generate_FUNCEND(quad* q);
void generate_RETURN(quad* q);
void generate_NEWTABLE(quad* q);
void generate_TABLEGETELEM(quad* q);
void generate_TABLESETELEM(quad* q);
void generate_NOP(quad* q);

static generator_func_t generators[] = {
    generate_ASSIGN,         /* assign_v        0    */
    generate_ADD,            /* add_v           1       */
    generate_SUB,            /* sub_v           2      */
    generate_MUL,            /* mul_v           3       */
    generate_DIV,            /* div_v           4       */
    generate_MOD,            /* mod_v           5       */
    generate_UMINUS,         /* uminus_v        6      */
    generate_AND,            /* and_v           7         */
    generate_OR,             /* or_v            8         */
    generate_NOT,            /* not_v           9        */
    generate_IF_EQ,          /* jeq_v           10       */
    generate_IF_NOTEQ,       /* jne_v           11      */
    generate_IF_LESSEQ,      /* jle_v           12     */
    generate_IF_GREATEREQ,   /* jge_v           13     */
    generate_IF_LESS,        /* jlt_v           14  */                                                         
    generate_IF_GREATER,     /* jgt_v           15  */
    generate_JUMP,           /* jump_v          16    */
    generate_CALL,           /* call_v          17  */
    generate_PARAM,          /* pusharg_v       18  */
    generate_FUNCSTART,      /* funcenter_v     19    */
    generate_FUNCEND,        /* funcexit_v      20  */
    generate_NEWTABLE,       /* newtable_v      21  */
    generate_TABLEGETELEM,   /* tablegetelem_v  22   */
    generate_TABLESETELEM,   /* tablesetelem_v  23     */
    generate_NOP,            /* nop_v           24       */
    generate_GETRETVAL       /* getretval_v     25     */
};


typedef struct return_list_node {
    unsigned instrNo;
    struct return_list_node* next;
} return_list_node;


/* Stack of return lists, one per function (for nested functions) */
static return_list_node* return_lists[1024]; /* adjust size as needed */
static int return_list_top = -1;

/* Helper to push a new return list when entering a function */
static void push_return_list() {
    return_lists[++return_list_top] = NULL;
}

/* Helper to pop the current return list when exiting a function */
static return_list_node* pop_return_list() {
    return return_lists[return_list_top--];
}

/* Helper to add a return jump to the current function's return list */
static void add_return_jump(unsigned instrNo) {
    return_list_node* node = malloc(sizeof(return_list_node));
    node->instrNo = instrNo;
    node->next = return_lists[return_list_top];
    return_lists[return_list_top] = node;
}

/* Helper to patch all return jumps at FUNCEND  */
static void patch_return_jumps(unsigned label) {
    return_list_node* node = return_lists[return_list_top];
    while (node) {
        instructions[node->instrNo].result.val = label;
        node = node->next;
    }
}

/* Helper: emit an instruction      */
static void emit_instruction(instruction* t) {
    if (currInstruction == totalInstructions) {
        totalInstructions = totalInstructions ? totalInstructions * 2 : INITIAL_INSTR_SIZE;
        instructions = realloc(instructions, totalInstructions * sizeof(instruction));
    }
    instructions[currInstruction++] = *t;
}

/* Helper: reset a vmarg    */
static void reset_operand(vmarg* arg) {
    arg->type = nil_a;
    arg->val = 0;
}

/* Helper: add incomplete jump  */
static void add_incomplete_jump(unsigned instrNo, unsigned iaddress) {
    incomplete_jump* ij = malloc(sizeof(incomplete_jump));
    ij->instrNo = instrNo;
    ij->iaddress = iaddress;
    ij->next = ij_head;
    ij_head = ij;
}

/* Helper: patch incomplete jumps   */
static void patch_incomplete_jumps(void) {
    incomplete_jump* ij = ij_head;
    while (ij) {
        unsigned target = (ij->iaddress == curr_quad) ? currInstruction : quads[ij->iaddress].taddress;
        instructions[ij->instrNo].result.val = target;
        ij = ij->next;
    }
}

/* Converts expr* to vmarg* */
void make_operand(expr* e, vmarg* arg) {
    if (!e) {
        printf("ERROR: make_operand called with NULL expr!\n");
        reset_operand(arg);
        return;
    }
    switch (e->type) {
        case var_e: case tableitem_e: case arithexpr_e: case boolexpr_e: case newtable_e:
            if (!e->sym) {
                printf("ERROR: expr->sym is NULL for expr type %d\n", e->type);
                reset_operand(arg);
                return;
            }
            arg->val = e->sym->offset;
            switch (e->sym->space) {
                case programvar: arg->type = global_a; break;
                case functionlocal: arg->type = local_a; break;
                case formalarg: arg->type = formal_a; break;
                case tempvar:       arg->type = temp_a;   break; 
                default:
                    printf("ERROR: Unknown sym->space %d\n", e->sym->space);
                    assert(0);
            }
            break;
        case constbool_e:
            arg->val = e->boolConst;
            arg->type = bool_a;
            break;
        case conststring_e:
            arg->val = consts_newstring(e->strConst);
            arg->type = string_a;
            break;
        case constnum_e:
            arg->val = consts_newnumber(e->numConst);
            arg->type = number_a;
            break;
        case nil_e:
            arg->type = nil_a;
            break;
        case programfunc_e:
            if (!e->sym) {
                printf("ERROR: programfunc_e expr->sym is NULL\n");
                reset_operand(arg);
                return;
            }
            arg->type = userfunc_a;
            arg->val = userfuncs_newfunc((userfunc){e->sym->taddress, e->sym->totallocals, e->sym->name});
            break;
        case libraryfunc_e:
            if (!e->sym) {
                printf("ERROR: libraryfunc_e expr->sym is NULL\n");
                reset_operand(arg);
                return;
            }
            if (!e->sym->name) {
                printf("ERROR: libraryfunc_e expr->sym->name is NULL\n");
                reset_operand(arg);
                return;
            }
            arg->type = string_a;
            arg->val = consts_newstring(e->sym->name);
            break;
        default:
            printf("ERROR: Unknown expr type %d\n", e->type);
            assert(0);
    }
}


/* Generates an assign instruction from a quad and emits it. */
void generate_ASSIGN(quad* q) {
    instruction t;
    t.opcode = assign_v;
    make_operand(q->result, &t.result);   
    make_operand(q->arg1, &t.arg1);       
    reset_operand(&t.arg2);              
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a jump instruction from a quad and emits it. */
void generate_JUMP(quad* q) {
    instruction t;
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a conditional jump (if equal) instruction from a quad and emits it. */
void generate_IF_EQ(quad* q) {
    instruction t;
    t.opcode = jeq_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a conditional jump (if not equal) instruction from a quad and emits it. */
void generate_IF_NOTEQ(quad* q) {
    instruction t;
    t.opcode = jne_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a conditional jump (if greater) instruction from a quad and emits it. */
void generate_IF_GREATER(quad* q) {
    instruction t;
    t.opcode = jgt_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a conditional jump (if greater or equal) instruction from a quad and emits it. */
void generate_IF_GREATEREQ(quad* q) {
    instruction t;
    t.opcode = jge_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a conditional jump (if less) instruction from a quad and emits it. */
void generate_IF_LESS(quad* q) {
    instruction t;
    t.opcode = jlt_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a conditional jump (if less or equal) instruction from a quad and emits it. */
void generate_IF_LESSEQ(quad* q) {
    instruction t;
    t.opcode = jle_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    t.result.type = label_a;
    if (q->label < currInstruction)
        t.result.val = quads[q->label].taddress;
    else
        add_incomplete_jump(currInstruction, q->label);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a push argument instruction from a quad and emits it. */
void generate_PARAM(quad* q) {
    instruction t;
    t.opcode = pusharg_v;
    make_operand(q->arg1, &t.arg1);
    reset_operand(&t.arg2);
    reset_operand(&t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a call instruction from a quad and emits it. */
void generate_CALL(quad* q) {
    if (!q->arg1 || !q->arg1->sym) {
        fprintf(stderr, "generate_CALL: ERROR - invalid function expression (line %d)\n", q->line);
        exit(1);
    }

  //  printf("generate_CALL: expr type = %d, name = %s\n", 
       // q->arg1 ? q->arg1->type : -1, 
      //  q->arg1 && q->arg1->sym ? q->arg1->sym->name : "(null)");

    instruction t;
    t.opcode = call_v;

    // --- ΕΛΕΓΧΟΣ για library function ---
    if (q->arg1->type == libraryfunc_e) {
        unsigned libfunc_index = lookup_or_insert_libfunc(q->arg1->sym->name); // υλοποίησέ το αν δεν υπάρχει
        t.result.type = libfunc_a;
        t.result.val = libfunc_index;
    } else {
        make_operand(q->arg1, &t.result);
    }

   // printf("DEBUG: generate_CALL result.type=%d (should be 9 for libfunc_a)\n", t.result.type);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Sets a vmarg to represent the retval operand. */
void make_retvaloperand(vmarg* arg) {
    arg->type = retval_a;
    arg->val = 0;
}


/* Generates a getretval instruction from a quad and emits it. */
void generate_GETRETVAL(quad* q) {
    instruction t;
    t.opcode = getretval_v;
    make_operand(q->result, &t.result);
    make_retvaloperand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
  //  printf("DEBUG: generate_GETRETVAL: result type=%d, val=%u, line=%u\n", t.result.type, t.result.val, t.srcLine);
    emit_instruction(&t);
    
}



/* Generates the start of a user function: emits a jump over the body, funcenter, and manages return list. */
void generate_FUNCSTART(quad* q) {
    functionLocalOffset = 0;
    formalArgOffset = 0;
    temp_var_offset = 0;
    if (!q || !q->result || !q->result->sym) {
      /*  fprintf(stderr, "generate_FUNCSTART: Invalid or missing function symbol.\n"); */
        return;
    }

    if (q->result->sym->type != USER_FUNCTION) {
      /*  fprintf(stderr, "generate_FUNCSTART: Skipping non-user function: %s (type=%d)\n",
            q->result->sym->name, q->result->sym->type);  */
        return;
    }

   
    instruction t;
    /*  Emit a jump to skip the function body unless called   */
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    t.result.val = 0; 
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);

   
    add_return_jump(q->taddress);

    t.opcode = funcenter_v;
    make_operand(q->result, &t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

    
    push_return_list();
}



/* Generates the end of a user function: patches return jumps and emits funcexit. */
void generate_FUNCEND(quad* q) {
    
    patch_return_jumps(currInstruction);
    assert(q->result && q->result->sym && q->result->sym->type == USER_FUNCTION);

    instruction t;
    t.opcode = funcexit_v;
    make_operand(q->result, &t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a return instruction: assigns to retval and emits a jump to be patched at FUNCEND. */
void generate_RETURN(quad* q) {
    instruction t;
    t.opcode = assign_v;
    make_retvaloperand(&t.result);
    make_operand(q->arg1, &t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);

    /* Now emit a jump    */
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a;
    t.result.val = 0; 
    emit_instruction(&t);

    /* currInstruction-1 to the current function's return list for patching at FUNCEND  */
      add_return_jump(currInstruction - 1);
}

/* Generates a newtable instruction from a quad and emits it. */
void generate_NEWTABLE(quad* q) {
    instruction t;
    t.opcode = newtable_v;
    make_operand(q->result, &t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a tablegetelem instruction from a quad and emits it. */
void generate_TABLEGETELEM(quad* q) {
    instruction t;
    t.opcode = tablegetelem_v;
    make_operand(q->arg1, &t.arg1);     
    make_operand(q->arg2, &t.arg2);     
    make_operand(q->result, &t.result); 
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates a tablesetelem instruction from a quad and emits it. */
void generate_TABLESETELEM(quad* q) {
    instruction t;
    t.opcode = tablesetelem_v;
    make_operand(q->result, &t.result);
    make_operand(q->arg1, &t.arg1);    
    make_operand(q->arg2, &t.arg2);   
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}




/* Generates code for logical AND (&&) operation with short-circuit evaluation. */
void generate_AND(quad* q) {
    /* result = arg1 && arg2    */
    unsigned label_false = currInstruction + 3;
    unsigned label_end = currInstruction + 5;

    /* if (!arg1) goto label_false  */
    instruction t;
    t.opcode = jeq_v;
    make_operand(q->arg1, &t.arg1);
    t.arg2.type = bool_a; t.arg2.val = 0;
    t.result.type = label_a; t.result.val = label_false;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* if (!arg2) goto label_false  */
    t.opcode = jeq_v;
    make_operand(q->arg2, &t.arg1);
    t.arg2.type = bool_a; t.arg2.val = 0;
    t.result.type = label_a; t.result.val = label_false;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* result = true    */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 1;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

    /* jump label_end   */
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a; t.result.val = label_end;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* label_false: result = false  */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 0;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

}




/* Generates code for logical OR (||) operation with short-circuit evaluation. */
void generate_OR(quad* q) {
    /* result = arg1 || arg2    */
    unsigned label_true = currInstruction + 3;
    unsigned label_end = currInstruction + 5;

    /* if (arg1) goto label_true    */
    instruction t;
    t.opcode = jeq_v;
    make_operand(q->arg1, &t.arg1);
    t.arg2.type = bool_a; t.arg2.val = 1;
    t.result.type = label_a; t.result.val = label_true;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* if (arg2) goto label_true    */
    t.opcode = jeq_v;
    make_operand(q->arg2, &t.arg1);
    t.arg2.type = bool_a; t.arg2.val = 1;
    t.result.type = label_a; t.result.val = label_true;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* result = false   */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 0;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

    /* jump label_end   */
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a; t.result.val = label_end;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* label_true: result = true    */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 1;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

}



/* Generates code for logical NOT (!) operation with branching. */
void generate_NOT(quad* q) {
    /* result = !arg1   */
    unsigned label_true = currInstruction + 2;
    unsigned label_end = currInstruction + 4;

    instruction t;
    /* if (!arg1) goto label_true   */
    t.opcode = jeq_v;
    make_operand(q->arg1, &t.arg1);
    t.arg2.type = bool_a; t.arg2.val = 0;
    t.result.type = label_a; t.result.val = label_true;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* result = false   */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 0;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

    /* jump label_end   */
    t.opcode = jump_v;
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.result.type = label_a; t.result.val = label_end;
    t.srcLine = q->line;
    emit_instruction(&t);

    /* label_true: result = true    */
    t.opcode = assign_v;
    make_operand(q->result, &t.result);
    t.arg1.type = bool_a; t.arg1.val = 1;
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    emit_instruction(&t);

    /* label_end: (no-op)   */
}


/* Generates a no-operation (NOP) instruction from a quad and emits it. */
void generate_NOP(quad* q) {
    instruction t;
    t.opcode = nop_v;
    reset_operand(&t.result);
    reset_operand(&t.arg1);
    reset_operand(&t.arg2);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates an add instruction from a quad and emits it. */
void generate_ADD(quad* q) {
    instruction t;
    t.opcode = add_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}



/* Generates a sub instruction from a quad and emits it. */
void generate_SUB(quad* q) {
    instruction t;
    t.opcode = sub_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a mul instruction from a quad and emits it. */
void generate_MUL(quad* q) {
    instruction t;
    t.opcode = mul_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}



/* Generates a div instruction from a quad and emits it. */
void generate_DIV(quad* q) {
    instruction t;
    t.opcode = div_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}


/* Generates a mod instruction from a quad and emits it. */
void generate_MOD(quad* q) {
    instruction t;
    t.opcode = mod_v;
    make_operand(q->arg1, &t.arg1);
    make_operand(q->arg2, &t.arg2);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}



/* Generates a unary minus (uminus) instruction from a quad and emits it. */
void generate_UMINUS(quad* q) {
    instruction t;
    t.opcode = mul_v;
    make_operand(q->arg1, &t.arg1);
    /* Create a constant -1 and use it as arg2  */
    t.arg2.type = number_a;
    t.arg2.val = consts_newnumber(-1.0);
    make_operand(q->result, &t.result);
    t.srcLine = q->line;
    q->taddress = currInstruction;
    emit_instruction(&t);
}

/* Generates the final VM code and writes it to the given file. */
void generate_final_code(const char* filename) {
    init_const_tables();

    currInstruction = 0;
    totalInstructions = INITIAL_INSTR_SIZE;
    instructions = malloc(totalInstructions * sizeof(instruction));
    if (!instructions) {
        printf("ERROR: instructions malloc failed\n");
        return;
    }

    if (!quads) {
        printf("ERROR: quads is NULL\n");
        free(instructions);
        return;
    }

    for (unsigned i = 0; i < curr_quad; ++i) {
     //   printf("DEBUG: quad[%u] op=%d\n", i, quads[i].op); 
        if (quads[i].op < 0 || quads[i].op >= sizeof(generators)/sizeof(generators[0])) {
          //  printf("DEBUG: quad[%u] is GETRETVAL\n", i); 
            printf("ERROR: Invalid quad op %d at quad %u\n", quads[i].op, i);
            free(instructions);
            return;
        }
        if (!generators[quads[i].op]) {
            printf("ERROR: generators[%d] is NULL (op=%d) at quad %u\n", quads[i].op, quads[i].op, i);
            free(instructions);
            return;
        }
        if (quads[i].op == funcenter_v || quads[i].op == funcexit_v) {
            if (!(quads[i].result && quads[i].result->sym && quads[i].result->sym->type == USER_FUNCTION)) {
                printf("WARNING: Skipping funcenter/funcexit quad %u with invalid result!\n", i);
                continue;
            }
        }
        if (quads[i].op == pusharg_v) {
         //   printf("PRE-GENERATE: quad[%u] param arg1 type=%d, name=%s\n",
              //  i,
             //   quads[i].arg1 ? quads[i].arg1->type : -1,
             //   (quads[i].arg1 && quads[i].arg1->sym) ? quads[i].arg1->sym->name : "(null)");
        }
        quads[i].taddress = currInstruction;
        generators[quads[i].op](&quads[i]);
    }
    patch_incomplete_jumps();

    FILE* out = fopen(filename, "w");
    if (!out) {
        perror("Cannot open output file");
        free(instructions);
        return;
    }

    fprintf(out, "Numbers: %u\n", totalNumConsts);
    for (unsigned i = 0; i < totalNumConsts; ++i)
        fprintf(out, "%u: %lf\n", i, numConsts[i]);


    fprintf(out, "Instructions: %u\n", currInstruction);
    for (unsigned i = 0; i < currInstruction; ++i) {
        fprintf(out, "%u: %d %d,%u %d,%u %d,%u\n", i,
            instructions[i].opcode,
            instructions[i].result.type, instructions[i].result.val,
            instructions[i].arg1.type, instructions[i].arg1.val,
            instructions[i].arg2.type, instructions[i].arg2.val
        );
    }

    fclose(out);
}




/* Writes the binary output of the VM code and constants to the given file. */
void write_binary_output(const char* filename) {
    FILE* out = fopen(filename, "wb");
    if (!out) { perror("Cannot open binary output file"); return; }
    unsigned magic = 0xDEADBEEF;
    fwrite(&magic, sizeof(unsigned), 1, out);
    fwrite(&totalNumConsts, sizeof(unsigned), 1, out);
    if (totalNumConsts > 0 && numConsts == NULL) { printf("ERROR: numConsts is NULL!\n"); fclose(out); return; }
    fwrite(numConsts, sizeof(double), totalNumConsts, out);


    if (totalStringConsts > 0 && stringConsts == NULL) { printf("ERROR: stringConsts is NULL!\n"); fclose(out); return; }
    fwrite(&totalStringConsts, sizeof(unsigned), 1, out);
    for (unsigned i = 0; i < totalStringConsts; ++i) {
        if (stringConsts[i] == NULL) { printf("ERROR: stringConsts[%u] is NULL!\n", i); fclose(out); return; }
        unsigned len = strlen(stringConsts[i]);

        fwrite(&len, sizeof(unsigned), 1, out);
        fwrite(stringConsts[i], sizeof(char), len, out);
    }


    if (totalUserFuncs > 0 && userFuncs == NULL) { printf("ERROR: userFuncs is NULL!\n"); fclose(out); return; }
    fwrite(&totalUserFuncs, sizeof(unsigned), 1, out);
    for (unsigned i = 0; i < totalUserFuncs; ++i) {
        if (userFuncs[i].id == NULL) { printf("ERROR: userFuncs[%u].id is NULL!\n", i); fclose(out); return; }

        fwrite(&userFuncs[i].address, sizeof(unsigned), 1, out);
        fwrite(&userFuncs[i].localSize, sizeof(unsigned), 1, out);
        unsigned len = strlen(userFuncs[i].id);
        fwrite(&len, sizeof(unsigned), 1, out);
        fwrite(userFuncs[i].id, sizeof(char), len, out);
    }


    if (totalLibFuncs > 0 && libFuncs == NULL) {
        printf("ERROR: libFuncs is NULL!\n");
        fclose(out);
        return;
    }
    fwrite(&totalLibFuncs, sizeof(unsigned), 1, out);   

    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        if (libFuncs[i] == NULL) {
            printf("ERROR: libFuncs[%u] is NULL!\n", i);
            fclose(out);
            return;
        }
        unsigned len = strlen(libFuncs[i]);
        fwrite(&len, sizeof(unsigned), 1, out);
        fwrite(libFuncs[i], sizeof(char), len, out);
    }

    /* --- DEBUG PRINTS for instructions ---
    printf("DEBUG: currInstruction = %u\n", currInstruction);
    printf("DEBUG: instructions pointer = %p\n", (void*)instructions);        */
    if (currInstruction > 0 && instructions != NULL) {
      /*  printf("DEBUG: instructions[0].opcode = %u\n", instructions[0].opcode);   */
    }

    if (currInstruction > 0 && instructions == NULL) { printf("ERROR: instructions is NULL!\n"); fclose(out); return; }
    fwrite(&currInstruction, sizeof(unsigned), 1, out);
    for (unsigned i = 0; i < currInstruction; ++i) {
        fwrite(&instructions[i].opcode, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].result.type, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].result.val, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].arg1.type, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].arg1.val, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].arg2.type, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].arg2.val, sizeof(unsigned), 1, out);
        fwrite(&instructions[i].srcLine, sizeof(unsigned), 1, out);
    }

    fclose(out);
}