#include "avm.h"
#include <math.h>
#include "avm_utils.h"
#include "avm_tables.h"
#include "avm_libfuncs.h"
#include "avm_const_tables.h"
#include "avm_instruction.h"
extern unsigned pc;
extern unsigned executionFinished;
extern avm_memcell stack[];
extern unsigned top;
extern unsigned topsp;
extern avm_memcell ax, bx, cx, retval;
extern instruction* code;
extern unsigned totalActuals;
typedef double (*arithmetic_func_t)(double, double);

void execute_arithmetic(instruction* instr, arithmetic_func_t op);

userfunc* userfuncs_getfunc(unsigned index);
void avm_calllibfunc(char* id);



void execute_assign(instruction* instr);
void execute_add(instruction* instr);
void execute_sub(instruction* instr);
void execute_mul(instruction* instr);
void execute_div(instruction* instr);
void execute_mod(instruction* instr);
void execute_uminus(instruction* instr);
void execute_and(instruction* instr);
void execute_or(instruction* instr);
void execute_not(instruction* instr);
void execute_jeq(instruction* instr);
void execute_jne(instruction* instr);
void execute_jle(instruction* instr);
void execute_jge(instruction* instr);
void execute_jlt(instruction* instr);
void execute_jgt(instruction* instr);
void execute_jump(instruction* instr);
void execute_call(instruction* instr);
void execute_pusharg(instruction* instr);
void execute_funcenter(instruction* instr);
void execute_funcexit(instruction* instr);
void execute_newtable(instruction* instr);
void execute_tablegetelem(instruction* instr);
void execute_tablesetelem(instruction* instr);
void execute_nop(instruction* instr);
void execute_getretval(instruction* instr);

/* Dispatcher table (order match vmopcode enum)    */
execute_func_t executeFuncs[] = {
    execute_assign,         /*0 */
    execute_add,            /*1 */
    execute_sub,            /*2 */
    execute_mul,            /*3 */
    execute_div,            /*4 */
    execute_mod,            /*5 */
    execute_uminus,         /*6 */
    execute_and,            /*7 */
    execute_or,            /*8  */
    execute_not,           /*9  */
    execute_jeq,           /*10 */
    execute_jne,          /*11  */
    execute_jle,            /*12    */
    execute_jge,              /*13  */
    execute_jlt,              /*14  */
    execute_jgt,              /*15  */
    execute_jump,          /*16 */
    execute_call,          /*17 */
    execute_pusharg,       /*18 */
    execute_funcenter,         /*19 */
    execute_funcexit,           /*20    */
    execute_newtable,             /*21  */
    execute_tablegetelem,          /*22 */
    execute_tablesetelem,           /*23    */
    execute_nop,                   /*24 */
    execute_getretval       /*25    */
};

/* Helper for arithmetic operations */
typedef double (*arithmetic_func_t)(double, double);

double add_impl(double x, double y) { return x + y; }
double sub_impl(double x, double y) { return x - y; }
double mul_impl(double x, double y) { return x * y; }
double div_impl(double x, double y) { return x / y; }
double mod_impl(double x, double y) { return fmod(x, y); }

arithmetic_func_t arithmeticFuncs[] = {
    add_impl, sub_impl, mul_impl, div_impl, mod_impl
};

/* Executes an assignment instruction. */
void execute_assign(instruction* instr) {
    
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);
    assert(lv && rv);

   // printf("DEBUG: execute_assign: lv type=%d, rv type=%d, rv numVal=%f\n", lv->type, rv->type, rv->data.numVal);

    avm_assign(lv, rv);
  
  //  printf("DEBUG: execute_assign: lv type=%d, rv type=%d\n", lv->type, rv->type);
}

/* Arithmetic   */
void execute_add(instruction* instr)    { execute_arithmetic(instr, add_impl); }
void execute_sub(instruction* instr)    { execute_arithmetic(instr, sub_impl); }
void execute_mul(instruction* instr)    { execute_arithmetic(instr, mul_impl); }
void execute_div(instruction* instr)    { execute_arithmetic(instr, div_impl); }
void execute_mod(instruction* instr)    { execute_arithmetic(instr, mod_impl); }



void execute_arithmetic(instruction* instr, arithmetic_func_t op) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(lv && rv1 && rv2);
    if (rv1->type != number_m || rv2->type != number_m) {
        fprintf(stderr, "Runtime error: not a number in arithmetic\n");
     //   fprintf(stderr, "DEBUG:  arg1 type=%d, arg2 type=%d\n", rv1->type, rv2->type);
        executionFinished = 1;
        return;
    }
    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = op(rv1->data.numVal, rv2->data.numVal);
}

/* Executes a unary minus instruction. */
void execute_uminus(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);
    assert(lv && rv);
    if (rv->type != number_m) {
        fprintf(stderr, "Runtime error: not a number in uminus\n");
        executionFinished = 1;
        return;
    }
    avm_memcellclear(lv);
    lv->type = number_m;
    lv->data.numVal = -rv->data.numVal;
}

/* Executes a logical AND instruction. */
void execute_and(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(lv && rv1 && rv2);
    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = avm_tobool(rv1) && avm_tobool(rv2);
}

/* Executes a logical OR instruction. */
void execute_or(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(lv && rv1 && rv2);
    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = avm_tobool(rv1) || avm_tobool(rv2);
}

/* Executes a logical NOT instruction. */
void execute_not(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* rv = avm_translate_operand(&instr->arg1, &ax);
    assert(lv && rv);
    avm_memcellclear(lv);
    lv->type = bool_m;
    lv->data.boolVal = !avm_tobool(rv);
}

/* Comparison helpers   */
typedef unsigned char (*cmp_func_t)(double, double);

unsigned char jle_impl(double x, double y) { return x <= y; }
unsigned char jge_impl(double x, double y) { return x >= y; }
unsigned char jlt_impl(double x, double y) { return x < y; }
unsigned char jgt_impl(double x, double y) { return x > y; }

void execute_comparison(instruction* instr, cmp_func_t cmp) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(rv1 && rv2);
    if (rv1->type != number_m || rv2->type != number_m) {
        fprintf(stderr, "Runtime error: not a number in comparison\n");
        executionFinished = 1;
        return;
    }
    if (cmp(rv1->data.numVal, rv2->data.numVal))
        pc = instr->result.val;
    else
        ++pc;
}

void execute_jle(instruction* instr) { execute_comparison(instr, jle_impl); }
void execute_jge(instruction* instr) { execute_comparison(instr, jge_impl); }
void execute_jlt(instruction* instr) { execute_comparison(instr, jlt_impl); }
void execute_jgt(instruction* instr) { execute_comparison(instr, jgt_impl); }


/* Executes a 'jump if equal' instruction. */
void execute_jeq(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(rv1 && rv2);
    unsigned char result = 0;
    if (rv1->type == undef_m || rv2->type == undef_m) {
        fprintf(stderr, "Runtime error: undef involved in equality!\n");
        executionFinished = 1;
        return;
    } else if (rv1->type == nil_m || rv2->type == nil_m) {
        result = (rv1->type == nil_m && rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        result = (avm_tobool(rv1) == avm_tobool(rv2));
    } else if (rv1->type != rv2->type) {
        fprintf(stderr, "Runtime error: illegal comparison of different types\n");
        executionFinished = 1;
        return;
    } else if (rv1->type == number_m) {
        result = (rv1->data.numVal == rv2->data.numVal);
    } else if (rv1->type == string_m) {
        result = (strcmp(rv1->data.strVal, rv2->data.strVal) == 0);
    } else {
        result = (rv1 == rv2);
    }
    if (result)
        pc = instr->result.val;
    else
        ++pc;
}


/* Executes a 'jump if not equal' instruction. */
void execute_jne(instruction* instr) {
    avm_memcell* rv1 = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* rv2 = avm_translate_operand(&instr->arg2, &bx);
    assert(rv1 && rv2);
    unsigned char result = 0;
    if (rv1->type == undef_m || rv2->type == undef_m) {
        fprintf(stderr, "Runtime error: undef involved in inequality!\n");
        executionFinished = 1;
        return;
    } else if (rv1->type == nil_m || rv2->type == nil_m) {
        result = !(rv1->type == nil_m && rv2->type == nil_m);
    } else if (rv1->type == bool_m || rv2->type == bool_m) {
        result = (avm_tobool(rv1) != avm_tobool(rv2));
    } else if (rv1->type != rv2->type) {
        fprintf(stderr, "Runtime error: illegal comparison of different types\n");
        executionFinished = 1;
        return;
    } else if (rv1->type == number_m) {
        result = (rv1->data.numVal != rv2->data.numVal);
    } else if (rv1->type == string_m) {
        result = (strcmp(rv1->data.strVal, rv2->data.strVal) != 0);
    } else {
        result = (rv1 != rv2);
    }
    if (result)
        pc = instr->result.val;
    else
        ++pc;
}


void execute_jump(instruction* instr) {
    pc = instr->result.val;
}

/* Executes a function call instruction. */
void execute_call(instruction* instr) {
    avm_memcell func_cell; 
    avm_memcell* func = avm_translate_operand(&instr->result, &func_cell);
    assert(func);
    switch (func->type) {
        case userfunc_m:
            avm_callsaveenvironment();
            pc = func->data.funcVal;
            assert(code[pc].opcode == funcenter_v);
            break;
        case libfunc_m:
            avm_callsaveenvironment();
          //  printf("DEBUG: BEFORE CALL: top=%u, totalActuals=%u\n", top, totalActuals);
          //  printf("DEBUG: execute_call: func->data.libfuncVal=%p, '%s'\n", (void*)func->data.libfuncVal, func->data.libfuncVal ? func->data.libfuncVal : "(null)");
            avm_calllibfunc(func->data.libfuncVal);
            top += totalActuals;
            totalActuals = 0;
    break;
case string_m:
    avm_callsaveenvironment();
    avm_calllibfunc(func->data.strVal);
    top += totalActuals;
    totalActuals = 0;
    break;
        default:
            fprintf(stderr, "Runtime error: call: cannot bind to function! func->type=%d\n", func->type);
            executionFinished = 1;
    }
}

/* Push argument    */
void execute_pusharg(instruction* instr) {
    assert(top > 0);
    avm_memcell* arg = avm_translate_operand(&instr->arg1, &ax);
    assert(arg);
    --top;
    avm_assign(&stack[top], arg);
  //  printf("DEBUG: pusharg: stack[%u] type=%d, numVal=%.6f, totalActuals=%u\n", top, stack[top].type, stack[top].data.numVal, totalActuals+1);
    ++totalActuals;
}


/* Function enter/exit   */
void execute_funcenter(instruction* instr) {
    avm_memcell* func = avm_translate_operand(&instr->result, &ax);
    assert(func && func->type == userfunc_m);
    totalActuals = 0;
    userfunc* funcInfo = userfuncs_getfunc(func->data.funcVal);
    topsp = top;
    top = top - funcInfo->localSize;
}

void execute_funcexit(instruction* instr) {
    unsigned oldTop = top;
    top = avm_get_envvalue(topsp + AVM_SAVEDTOP_OFFSET);
    pc = avm_get_envvalue(topsp + AVM_SAVEDPC_OFFSET);
    topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);

    
    for (unsigned i = top; i < oldTop; ++i)
        avm_memcellclear(&stack[i]);
}

/* Executes a new table creation instruction. */
void execute_newtable(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
  //  printf("DEBUG: execute_newtable: lv offset/type=%d/%d\n", instr->result.val, lv->type);
    assert(lv);
    avm_memcellclear(lv);
    lv->type = table_m;
    lv->data.tableVal = avm_tablenew();
    avm_tableincrefcounter(lv->data.tableVal);
}


/* Executes a table element retrieval instruction. */
void execute_tablegetelem(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    avm_memcell* t = avm_translate_operand(&instr->arg1, NULL);
    avm_memcell* i = avm_translate_operand(&instr->arg2, &ax);
    assert(lv && t && i);
    avm_memcellclear(lv);
    lv->type = nil_m;
    if (t->type != table_m) {
        avm_error("illegal use of type as table!");
        return;
    }
    avm_memcell* content = avm_tablegetelem(t->data.tableVal, i);
    if (content)
        avm_assign(lv, content);
}


/* Executes a table element set instruction. */
void execute_tablesetelem(instruction* instr) {
    avm_memcell* t = avm_translate_operand(&instr->result, NULL);
 //   printf("DEBUG: execute_tablesetelem: t offset/type=%d/%d\n", instr->result.val, t->type);
    avm_memcell* i = avm_translate_operand(&instr->arg1, &ax);
    avm_memcell* c = avm_translate_operand(&instr->arg2, &bx);
 //   printf("DEBUG: execute_tablesetelem: key type=%d, content type=%d\n", i->type, c->type);
    assert(t && i && c);
    if (t->type != table_m) {
      //  printf("DEBUG: execute_tablesetelem: t->type=%d (expected %d)\n", t->type, table_m);
        avm_error("illegal use of type as table!");
        return;
    }
   // printf("DEBUG: execute_tablesetelem: calling avm_tablesetelem\n");
    avm_tablesetelem(t->data.tableVal, i, c);
  //  printf("DEBUG: execute_tablesetelem: finished avm_tablesetelem\n");
}


/* Executes a no-operation instruction. */
void execute_nop(instruction* instr) {
    /* Do nothing   */
}


/* Executes a getretval instruction. */
void execute_getretval(instruction* instr) {
    avm_memcell* lv = avm_translate_operand(&instr->result, NULL);
    assert(lv);
    avm_assign(lv, &retval);
  //  printf("DEBUG: GETRETVAL: retval.type=%d, retval.numVal=%.6f -> lv.type=%d, lv.numVal=%.6f\n",
     //   retval.type, retval.data.numVal, lv->type, lv->data.numVal);
}