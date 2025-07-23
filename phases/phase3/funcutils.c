#include <stdlib.h>
#include "quads.h"
#include "funcutils.h"
#include "expr.h"
#include "temp.h"

extern quad* quads;

// Node for a single return jump (stores the quad index)
typedef struct return_jump_node {
    int quad;
    struct return_jump_node* next;
} return_jump_node;
// List of return jumps for a function, with stack linkage for nesting
typedef struct return_jump_list {
    return_jump_node* head;
    struct return_jump_list* prev;
} return_jump_list;
// Top of the return jump stack (one list per active function)
static return_jump_list* return_stack = NULL;
// Push a new (empty) return jump list onto the stack (for a new function)
void push_return_list(void) {
    return_jump_list* newlist = malloc(sizeof(return_jump_list));
    newlist->head = NULL;
    newlist->prev = return_stack;
    return_stack = newlist;
}
// Pop and free the top return jump list from the stack
void pop_return_list(void) {
    if (return_stack) {
        return_jump_node* node = return_stack->head;
        while (node) {
            return_jump_node* tmp = node;
            node = node->next;
            free(tmp);
        }
        return_jump_list* tmp = return_stack;
        return_stack = return_stack->prev;
        free(tmp);
    }
}
// Add a return jump (quad index) to the current function's return jump list
void add_return_jump(int quad) {
    if (!return_stack) push_return_list();
    return_jump_node* node = malloc(sizeof(return_jump_node));
    node->quad = quad;
    node->next = return_stack->head;
    return_stack->head = node;
}
// Patch all return jumps in the current function to jump to 'label' (function end)
void patch_return_jumps(int label) {
    if (!return_stack) return;
    return_jump_node* node = return_stack->head;
    while (node) {
        quads[node->quad].label = label;
        node = node->next;
    }
    // Καθάρισε τη λίστα
    node = return_stack->head;
    while (node) {
        return_jump_node* tmp = node;
        node = node->next;
        free(tmp);
    }
    return_stack->head = NULL;
}