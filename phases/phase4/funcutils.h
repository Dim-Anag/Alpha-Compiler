#ifndef FUNCUTILS_H
#define FUNCUTILS_H
// Add a return jump (quad index) to the current function's return jump list
void add_return_jump(int quad);
// Patch all return jumps in the current function to jump to 'label' (function end)
void patch_return_jumps(int label);
// Push a new (empty) return jump list onto the stack (for a new function)
void push_return_list(void);
// Pop and free the top return jump list from the stack
void pop_return_list(void);

#endif