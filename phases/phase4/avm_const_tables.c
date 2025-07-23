#include "avm_const_tables.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "avm_instruction.h"
#define INITIAL_SIZE 32


double* numConsts = NULL;
unsigned totalNumConsts = 0, numConstsCapacity = 0;

char** stringConsts = NULL;
unsigned totalStringConsts = 0, stringConstsCapacity = 0;

userfunc* userFuncs = NULL;
unsigned totalUserFuncs = 0, userFuncsCapacity = 0;

char** libFuncs = NULL;
unsigned totalLibFuncs = 0, libFuncsCapacity = 0;



/* Prints the current library functions for debugging. */
void print_libfuncs_debug(const char* msg) {
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
       // printf("  libFuncs[%u] = '%s' (%p)\n", i, libFuncs[i], (void*)libFuncs[i]);
    }
}

/* Initializes all constant tables (numbers, strings, user functions, library functions). */
void init_const_tables(void) {
    numConstsCapacity = stringConstsCapacity = userFuncsCapacity = libFuncsCapacity = INITIAL_SIZE;

    numConsts = (double*)malloc(numConstsCapacity * sizeof(double));
    stringConsts = (char**)malloc(stringConstsCapacity * sizeof(char*));
    userFuncs = (userfunc*)malloc(userFuncsCapacity * sizeof(userfunc));
    print_libfuncs_debug("After init_const_tables");
}



/* Frees all memory used by the constant tables. */
void free_const_tables(void) {
    print_libfuncs_debug("Before free_const_tables");
    for (unsigned i = 0; i < totalStringConsts; ++i)
        free(stringConsts[i]);
    for (unsigned i = 0; i < totalLibFuncs; ++i)
        free(libFuncs[i]);
    for (unsigned i = 0; i < totalUserFuncs; ++i)
        free(userFuncs[i].id);
    free(numConsts);
    free(stringConsts);
    free(userFuncs);
    free(libFuncs);
}

/* Returns the index of number constant 'n', adding it if not present. */
unsigned consts_newnumber(double n) {
    for (unsigned i = 0; i < totalNumConsts; ++i)
        if (numConsts[i] == n)
            return i;
    if (totalNumConsts == numConstsCapacity) {
        numConstsCapacity *= 2;
        numConsts = (double*)realloc(numConsts, numConstsCapacity * sizeof(double));
    }
    numConsts[totalNumConsts] = n;
    return totalNumConsts++;
  //  printf("DEBUG: consts_newnumber: n=%f, index=%u\n", n, totalNumConsts);
}


/* Returns the index of user function 'f', adding it if not present. */
unsigned consts_newstring(const char* s) {
    for (unsigned i = 0; i < totalStringConsts; ++i)
        if (strcmp(stringConsts[i], s) == 0)
            return i;
    if (totalStringConsts == stringConstsCapacity) {
        stringConstsCapacity *= 2;
        stringConsts = (char**)realloc(stringConsts, stringConstsCapacity * sizeof(char*));
    }
    stringConsts[totalStringConsts] = strdup(s);
    return totalStringConsts++;
}

/* Returns the index of user function 'f', adding it if not present. */
unsigned userfuncs_newfunc(userfunc f) {
    for (unsigned i = 0; i < totalUserFuncs; ++i)
        if (strcmp(userFuncs[i].id, f.id) == 0 && userFuncs[i].address == f.address)
            return i;
    if (totalUserFuncs == userFuncsCapacity) {
        userFuncsCapacity *= 2;
        userFuncs = (userfunc*)realloc(userFuncs, userFuncsCapacity * sizeof(userfunc));
    }
    userFuncs[totalUserFuncs].address = f.address;
    userFuncs[totalUserFuncs].localSize = f.localSize;
    userFuncs[totalUserFuncs].id = strdup(f.id);
    return totalUserFuncs++;
}


/* Returns the index of library functions, adding it if not present. */
unsigned libfuncs_newused(const char* s) {
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        if (strcmp(libFuncs[i], s) == 0) {
            print_libfuncs_debug("After libfuncs_newused (existing)");
            return i;
        }
    }
    if (totalLibFuncs == libFuncsCapacity) {
        unsigned oldCapacity = libFuncsCapacity;
        libFuncsCapacity = libFuncsCapacity ? libFuncsCapacity * 2 : 8;
        char **tmp = realloc(libFuncs, libFuncsCapacity * sizeof(char*));
        if (!tmp) {
            fprintf(stderr, "ERROR: realloc failed in libfuncs_newused\n");
            exit(1);
        }
        libFuncs = tmp;
        print_libfuncs_debug("After realloc in libfuncs_newused");
    }
    libFuncs[totalLibFuncs] = strdup(s);
    if (!libFuncs[totalLibFuncs]) {
        fprintf(stderr, "ERROR: strdup failed in libfuncs_newused\n");
        exit(1);
    }
    print_libfuncs_debug("After libfuncs_newused (added)");
    return totalLibFuncs++;
}


/* Initializes the built-in library functions in the libFuncs table. */
void initialize_libfuncs() {
    const char* builtin_libfuncs[] = {
        "print", 
        "input", 
        "objectmemberkeys", 
        "objecttotalmembers", 
        "objectcopy",
        "totalarguments", 
        "argument", 
        "typeof", 
        "strtonum", 
        "sqrt", 
        "cos", 
        "sin"
    };
    const unsigned builtin_libfuncs_count = sizeof(builtin_libfuncs) / sizeof(builtin_libfuncs[0]);

    libFuncsCapacity = builtin_libfuncs_count > 8 ? builtin_libfuncs_count : 8;
    libFuncs = malloc(libFuncsCapacity * sizeof(char*));
    if (!libFuncs) {
        fprintf(stderr, "ERROR: malloc failed for libFuncs\n");
        exit(1);
    }
    totalLibFuncs = builtin_libfuncs_count;
    for (unsigned i = 0; i < builtin_libfuncs_count; ++i) {
        libFuncs[i] = strdup(builtin_libfuncs[i]);
        if (!libFuncs[i]) {
            fprintf(stderr, "ERROR: strdup failed for libFuncs[%u]\n", i);
            exit(1);
        }
    }

    print_libfuncs_debug("After libFuncs init");
}

unsigned lookup_or_insert_libfunc(const char* name) {
    for (unsigned i = 0; i < totalLibFuncs; ++i) {
        if (strcmp(libFuncs[i], name) == 0)
            return i;
    }
    // Αν δεν υπάρχει, πρόσθεσέ το
    if (totalLibFuncs == libFuncsCapacity) {
        libFuncsCapacity = libFuncsCapacity ? libFuncsCapacity * 2 : 8;
        libFuncs = realloc(libFuncs, libFuncsCapacity * sizeof(char*));
        if (!libFuncs) {
            fprintf(stderr, "ERROR: realloc failed for libFuncs\n");
            exit(1);
        }
    }
    libFuncs[totalLibFuncs] = strdup(name);
    if (!libFuncs[totalLibFuncs]) {
        fprintf(stderr, "ERROR: strdup failed for libFuncs[%u]\n", totalLibFuncs);
        exit(1);
    }
    return totalLibFuncs++;
}