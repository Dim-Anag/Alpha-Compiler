#include "avm.h"
#include "avm_instruction.h"
#include <assert.h>
// Global variables
avm_memcell stack[AVM_STACKSIZE];
avm_memcell ax, bx, cx, retval;
unsigned top = AVM_STACKSIZE - 1;
unsigned topsp = AVM_STACKSIZE - 1;
unsigned pc = 0;
unsigned executionFinished = 0;
unsigned codeSize = 0;
instruction* code = NULL;

/* Forward declarations for loading  */
int load_constants_and_code(void);

/* Initializes the AVM stack, registers, and loads program constants/code. */
void avm_initialize(void) {
    printf("Starting VM\n");
   
    for (unsigned i = 0; i < AVM_STACKSIZE; ++i) {
        stack[i].type = undef_m;
    }
    ax.type = bx.type = cx.type = retval.type = undef_m;
    top = topsp = AVM_STACKSIZE - 1;
    pc = 0;
    executionFinished = 0;


    if (!load_constants_and_code()) {
        fprintf(stderr, "Failed to load program binary.\n");
        exit(1);
    }
}


/* Main VM execution loop: fetches and executes instructions until finished. */
void execute_cycle(void) {
    while (!executionFinished && pc < codeSize) {
        instruction* instr = code + pc;
        unsigned oldPC = pc;
        extern execute_func_t executeFuncs[];
       assert(instr->opcode >= 0 && instr->opcode <= getretval_v);
        (*executeFuncs[instr->opcode])(instr);
        if (pc == oldPC)
            ++pc;
    }
}

int main(void) {
    avm_initialize();
    execute_cycle();
    return 0;
    printf("Execution finished.\n");
}