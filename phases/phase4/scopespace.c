#include <stdio.h>
#include <stdlib.h>
#include "scopespace.h"

#define STACK_SIZE 1024

// Offsets για κάθε scopespace
int formal_arg_offset = 0;
extern int local_var_offset;

// Stack για τα offsets (π.χ. για nested functions)
int offset_stack[STACK_SIZE];
int offset_stack_top = -1;

void* scopeoffsetstack = NULL; 

void enterscopespace(void) {
}

void exitscopespace(void) {
}

void resetformalargsoffset(void) {
    formal_arg_offset = 0;
}

void resetfunctionlocalsoffset(void) {
    local_var_offset = 0;
}

int currscopeoffset(void) {
    return local_var_offset;
}

void restorecurrscopeoffset(int offset) {
    local_var_offset = offset;
}

void push(void* dummy, int value) {
    (void)dummy; // unused
    if (offset_stack_top + 1 >= STACK_SIZE) {
        fprintf(stderr, "Offset stack overflow!\n");
        exit(1);
    }
    offset_stack[++offset_stack_top] = value;
}

int pop_and_top(void* dummy) {
    (void)dummy; // unused
    if (offset_stack_top < 0) {
        fprintf(stderr, "Offset stack underflow!\n");
        exit(1);
    }
    int value = offset_stack[offset_stack_top--];
    return value;
}