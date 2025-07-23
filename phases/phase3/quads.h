#ifndef QUADS_H
#define QUADS_H
//enum
typedef enum {
    assign, add, sub, mul, div_, mod, uminus,
    and_, or_, not_,
    if_eq, if_noteq, if_lesseq, if_greatereq, if_less, if_greater,
    jump,
    call, param, return_, getretval,
    funcstart, funcend,
    tablecreate, tablegetelem, tablesetelem
} iopcode;

struct expr; // Forward declaration
// Structure representing a single quad 
typedef struct quad {
    iopcode op;
    struct expr* result;
    struct expr* arg1;
    struct expr* arg2;
    unsigned label;
    unsigned line;
} quad;
// Emits a new quad 
void emit(iopcode op, struct expr* result, struct expr* arg1, struct expr* arg2, unsigned label, unsigned line);
// Returns the index of the next quad to be emitted
unsigned nextquad(void);
// Sets the label (target) of a specific quad (used for backpatching jumps)
void patchlabel(unsigned quadNo, unsigned label);
// Prints all generated quads 
void print_quads(const char* filename);
// Returns the label for the next quad (used for jumps)
unsigned nextquadlabel(void);
#endif