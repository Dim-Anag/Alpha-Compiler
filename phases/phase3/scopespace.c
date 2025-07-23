#include <stdio.h>
#include "scopespace.h"
//dummy implementations
void enterscopespace(void) {}
void exitscopespace(void) {}
void resetformalargsoffset(void) {}
void resetfunctionlocalsoffset(void) {}
int currscopeoffset(void) { return 0; }
void restorecurrscopeoffset(int offset) {}
void push(void* stack, int value) {}
int pop_and_top(void* stack) { return 0; }
void* scopeoffsetstack = NULL;