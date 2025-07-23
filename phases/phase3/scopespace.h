#ifndef SCOPESPACE_H
#define SCOPESPACE_H
//dummies
void enterscopespace(void);
void exitscopespace(void);
void resetformalargsoffset(void);
void resetfunctionlocalsoffset(void);
int currscopeoffset(void);
void restorecurrscopeoffset(int offset);
void push(void* stack, int value);
int pop_and_top(void* stack);
extern void* scopeoffsetstack;

#endif // SCOPESPACE_H