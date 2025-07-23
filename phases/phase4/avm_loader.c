#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avm_const_tables.h"
#include "avm_instruction.h"

extern double* numConsts;
extern unsigned totalNumConsts;

extern char** stringConsts;
extern unsigned totalStringConsts;

extern userfunc* userFuncs;
extern unsigned totalUserFuncs;

extern char** libFuncs;
extern unsigned totalLibFuncs;

extern instruction* code;
extern unsigned codeSize;




/* Loads constants and instructions from the binary file "program.abc" into memory.
   Returns 1 on success, 0 on failure. */
int load_constants_and_code(void) {
    FILE* in = fopen("program.abc", "rb");
    if (!in) {
        perror("Cannot open binary input file");
        return 0;
    }

    unsigned magic;
    fread(&magic, sizeof(unsigned), 1, in);
    if (magic != 0xDEADBEEF) {
        fprintf(stderr, "Invalid binary file (magic number mismatch)\n");
        fclose(in);
        return 0;
    }


    fread(&totalNumConsts, sizeof(unsigned), 1, in);

    if (totalNumConsts > 0) {
        numConsts = malloc(totalNumConsts * sizeof(double));
        fread(numConsts, sizeof(double), totalNumConsts, in);
       // for (unsigned i = 0; i < totalNumConsts; ++i)
         //   printf("DEBUG (VM): numConsts[%u] = %f\n", i, numConsts[i]);
    } else {
        numConsts = NULL;
    }


    fread(&totalStringConsts, sizeof(unsigned), 1, in);

    if (totalStringConsts > 0) {
        stringConsts = malloc(totalStringConsts * sizeof(char*));
        for (unsigned i = 0; i < totalStringConsts; ++i) {
            unsigned len;
            fread(&len, sizeof(unsigned), 1, in);
            stringConsts[i] = malloc(len + 1);
            fread(stringConsts[i], sizeof(char), len, in);
            stringConsts[i][len] = '\0';
        }
    } else {
        stringConsts = NULL;
    }


    fread(&totalUserFuncs, sizeof(unsigned), 1, in);

    if (totalUserFuncs > 0) {
        userFuncs = malloc(totalUserFuncs * sizeof(userfunc));
        for (unsigned i = 0; i < totalUserFuncs; ++i) {
            fread(&userFuncs[i].address, sizeof(unsigned), 1, in);
            fread(&userFuncs[i].localSize, sizeof(unsigned), 1, in);
            unsigned len;
            fread(&len, sizeof(unsigned), 1, in);
            userFuncs[i].id = malloc(len + 1);
            fread(userFuncs[i].id, sizeof(char), len, in);
            userFuncs[i].id[len] = '\0';
        }
    } else {
        userFuncs = NULL;
    }


    fread(&totalLibFuncs, sizeof(unsigned), 1, in);
    if (totalLibFuncs > 0) {
        libFuncs = malloc(totalLibFuncs * sizeof(char*));
        for (unsigned i = 0; i < totalLibFuncs; ++i) {
            unsigned len;
            fread(&len, sizeof(unsigned), 1, in);
            libFuncs[i] = malloc(len + 1);
            fread(libFuncs[i], sizeof(char), len, in);
            libFuncs[i][len] = '\0';
        }
       // for (unsigned i = 0; i < totalLibFuncs; ++i)
           // printf("VM: libFuncs[%u] = '%s'\n", i, libFuncs[i]);
    } else {
        libFuncs = NULL;
    }

    fread(&codeSize, sizeof(unsigned), 1, in);
    if (codeSize > 0) {
        code = malloc(codeSize * sizeof(instruction));
        for (unsigned i = 0; i < codeSize; ++i) {
            fread(&code[i].opcode, sizeof(unsigned), 1, in);
            fread(&code[i].result.type, sizeof(unsigned), 1, in);
            fread(&code[i].result.val, sizeof(unsigned), 1, in);
            fread(&code[i].arg1.type, sizeof(unsigned), 1, in);
            fread(&code[i].arg1.val, sizeof(unsigned), 1, in);
            fread(&code[i].arg2.type, sizeof(unsigned), 1, in);
            fread(&code[i].arg2.val, sizeof(unsigned), 1, in);
            fread(&code[i].srcLine, sizeof(unsigned), 1, in);
          /*  printf("DEBUG (VM): code[%u] = {opcode=%u, result.type=%u, result.val=%u, arg1.type=%u, arg1.val=%u, arg2.type=%u, arg2.val=%u, srcLine=%u}\n",
                i, code[i].opcode, code[i].result.type, code[i].result.val,
            code[i].arg1.type, code[i].arg1.val,
              code[i].arg2.type, code[i].arg2.val,
                code[i].srcLine);     */
        }
    } else {
        code = NULL;
    }

    fclose(in);
    return 1;
}