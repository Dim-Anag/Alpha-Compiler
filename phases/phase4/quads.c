#include <stdio.h>
#include <stdlib.h>
#include "quads.h"
#include "expr.h"
#define INITIAL_QUAD_SIZE 1024
const char* iopcode_to_string(iopcode op); 

quad* quads = NULL; //array of quads
unsigned total_quads = 0;
unsigned curr_quad = 0;

// Expands the quads array if needed (dynamic allocation)
static void expand_quads(void) {
    if (curr_quad == total_quads) {
        total_quads += INITIAL_QUAD_SIZE;
        quads = realloc(quads, total_quads * sizeof(quad));
        if (!quads) {
            fprintf(stderr, "Out of memory while expanding quads!\n");
            exit(1);
        }
    }
}
void init_quads(void) {
    total_quads = INITIAL_QUAD_SIZE;
    quads = malloc(total_quads * sizeof(quad));
    if (!quads) {
        fprintf(stderr, "Out of memory!\n");
        exit(1);
    }
    curr_quad = 0;
}

/* Emits a new quad (adds it to the quads array)    */
void emit(iopcode op, struct expr* result, struct expr* arg1, struct expr* arg2, unsigned label, unsigned line) {
   // printf("EMIT: quad[%u] op=%d\n", curr_quad, op); 
    expand_quads();
    quads[curr_quad].op = op;
    if (op == call) {
 /*   printf("emit: call quad, arg1 type = %d, name = %s\n",
        arg1 ? arg1->type : -1,
        arg1 && arg1->sym ? arg1->sym->name : "(null)");  */
    }
    quads[curr_quad].result = result ? copyexpr(result) : NULL;
    if (op == assign) {
  /*  printf("emit_assign: rvalue type=%d, strConst=%s\n",
        arg1 ? arg1->type : -1,
        (arg1 && arg1->type == conststring_e && arg1->strConst) ? arg1->strConst : "NULL");   */
}
    quads[curr_quad].arg1 = arg1 ? copyexpr(arg1) : NULL;
    /*
   printf("AFTER COPY: quad[%d] param arg1 ptr=%p, type=%d, sym=%p, name=%s\n",
    curr_quad,
    (void*)quads[curr_quad].arg1,
    quads[curr_quad].arg1 ? quads[curr_quad].arg1->type : -1,
    quads[curr_quad].arg1 ? (void*)quads[curr_quad].arg1->sym : NULL,
    (quads[curr_quad].arg1 && quads[curr_quad].arg1->sym) ? quads[curr_quad].arg1->sym->name : "(null)");
    */
    quads[curr_quad].arg2 = arg2 ? copyexpr(arg2) : NULL;
    quads[curr_quad].label = label;
    quads[curr_quad].line = line;

    
   /* fprintf(stderr, "DEBUG: emit quad#%u: %s, label=%u, line=%u\n", curr_quad+1, iopcode_to_string(op), label+1, line);   */
    if (op == funcstart) {
 /*   printf("emit: funcstart, result type = %d, name = %s, sym type = %d\n",
        result ? result->type : -1,
        result && result->sym ? result->sym->name : "(null)",
        result && result->sym ? result->sym->type : -1);      */
}
    if (op == param) {
    /*   printf("EMIT PARAM: expr type=%d, name=%s\n",
            arg1 ? arg1->type : -1,
            (arg1 && arg1->sym) ? arg1->sym->name : "(null)");    */
    }
    curr_quad++;
    /*
    printf("POST-EMIT: quad[%u] param arg1 ptr=%p, type=%d, sym=%p, name=%s\n",
    curr_quad-1,
    (void*)quads[curr_quad-1].arg1,
    quads[curr_quad-1].arg1 ? quads[curr_quad-1].arg1->type : -1,
    quads[curr_quad-1].arg1 ? (void*)quads[curr_quad-1].arg1->sym : NULL,
    (quads[curr_quad-1].arg1 && quads[curr_quad-1].arg1->sym) ? quads[curr_quad-1].arg1->sym->name : "(null)");   */
}








// Returns the index of the next quad to be emitted
unsigned nextquad(void) {
    return curr_quad;
}
// Sets the label (target) of a specific quad (used for backpatching jumps)
void patchlabel(unsigned quadNo, unsigned label) {
  //   fprintf(stderr, "DEBUG: patchlabel quad#%u to label %u\n", quadNo, label);
    if (quadNo < curr_quad) {
        quads[quadNo].label = label;
    }
}
// Converts an iopcode enum value to a string for printing
const char* iopcode_to_string(iopcode op) {
    switch(op) {
        case assign: return "ASSIGN";
        case add: return "ADD";
        case sub: return "SUB";
        case mul: return "MUL";
        case div_: return "DIV";
        case mod: return "MOD";
        case uminus: return "UMINUS";
        case and_: return "AND";
        case or_: return "OR";
        case not_: return "NOT";
        case if_eq: return "IF_EQ";
        case if_noteq: return "IF_NOTEQ";
        case if_lesseq: return "IF_LESSEQ";
        case if_greatereq: return "IF_GREATEREQ";
        case if_less: return "IF_LESS";
        case if_greater: return "IF_GREATER";
        case jump: return "JUMP";
        case call: return "CALL";
        case param: return "PARAM";
        case return_: return "RETURN";
        case getretval: return "GETRETVAL";
        case funcstart: return "FUNCSTART";
        case funcend: return "FUNCEND";
        case tablecreate: return "TABLECREATE";
        case tablegetelem: return "TABLEGETELEM";
        case tablesetelem: return "TABLESETELEM";
        default: return "UNKNOWN";
    }
}
// Prints
void print_expr(expr* e, FILE* f) {
    if (!e) {
        fprintf(f, "_");
        return;
    }
   // If the expr has a temp symbol (name starts with ^)
    if (e->sym && e->sym->name && e->sym->name[0] == '^') {
        fprintf(f, "%s", e->sym->name); 
        return;
    }
    switch (e->type) {
        case var_e:
        case tableitem_e:
        case programfunc_e:
        case libraryfunc_e:
        case member_e:
            if (e->sym && e->sym->name)
                fprintf(f, "%s", e->sym->name);
            else
                fprintf(f, "_");
            break;
        case constnum_e:
            fprintf(f, "%g", e->numConst);
            break;
        case conststring_e:
            fprintf(f, "\"%s\"", e->strConst);
            break;
        case constbool_e:
            fprintf(f, "%s", e->boolConst ? "true" : "false");
            break;
        case nil_e:
            fprintf(f, "nil");
            break;
        default:
            fprintf(f, "_");
    }
}




// Helper: print expr to buffer for width calculation
void expr_to_buf(expr* e, char* buf, size_t buflen) {
    if (!e) { buf[0] = 0; return; }
    if (e->sym && e->sym->name && e->sym->name[0] == '^') {
        snprintf(buf, buflen, "%s", e->sym->name);
        return;
    }
    switch (e->type) {
        case var_e:
        case tableitem_e:
        case programfunc_e:
        case libraryfunc_e:
        case member_e:
            if (e->sym && e->sym->name)
                snprintf(buf, buflen, "%s", e->sym->name);
            else
                buf[0] = 0;
            break;
        case constnum_e:
            snprintf(buf, buflen, "%g", e->numConst);
            break;
        case conststring_e:
            snprintf(buf, buflen, "\"%s\"", e->strConst);
            break;
        case constbool_e:
            snprintf(buf, buflen, "%s", e->boolConst ? "true" : "false");
            break;
        case nil_e:
            snprintf(buf, buflen, "nil");
            break;
        default:
            buf[0] = 0;
    }
}
// Prints all quads
void print_quads(const char* filename) {
    const char* headers[] = {"quad#", "opcode", "result", "arg1", "arg2", "label", "line"};
    const int ncols = 7;
    unsigned maxw[7] = {0};
    char buf[128];

    // 1st pass: find max width for each column
    for (int i = 0; i < ncols; ++i)
        maxw[i] = strlen(headers[i]);

    for (unsigned i = 0; i < curr_quad; ++i) {
        // quad#
        snprintf(buf, sizeof(buf), "%u", i + 1);
        if (strlen(buf) > maxw[0]) maxw[0] = strlen(buf);

        // opcode
        snprintf(buf, sizeof(buf), "%s", iopcode_to_string(quads[i].op));
        if (strlen(buf) > maxw[1]) maxw[1] = strlen(buf);

        // result
        buf[0] = 0; expr_to_buf(quads[i].result, buf, sizeof(buf));
        if (strlen(buf) > maxw[2]) maxw[2] = strlen(buf);

        // arg1
        buf[0] = 0; expr_to_buf(quads[i].arg1, buf, sizeof(buf));
        if (strlen(buf) > maxw[3]) maxw[3] = strlen(buf);

        // arg2
        buf[0] = 0; expr_to_buf(quads[i].arg2, buf, sizeof(buf));
        if (strlen(buf) > maxw[4]) maxw[4] = strlen(buf);

        // label
if (
    quads[i].op == jump ||
    (quads[i].op >= if_eq && quads[i].op <= if_greater) // adjust range if needed for your conditionals
) {
    snprintf(buf, sizeof(buf), "%u", quads[i].label + 1);
} else {
    buf[0] = 0;
}
if (strlen(buf) > maxw[5]) maxw[5] = strlen(buf);

        // line
        snprintf(buf, sizeof(buf), "[line %u]", quads[i].line);
        if (strlen(buf) > maxw[6]) maxw[6] = strlen(buf);
    }

    FILE* f = fopen(filename, "w");
    if (!f) {
        perror("Cannot open quads.txt");
        return;
    }

    // Print header
    for (int i = 0; i < ncols; ++i)
        fprintf(f, "%-*s ", maxw[i], headers[i]);
    fprintf(f, "\n");

    // Print separator
    for (int i = 0; i < ncols; ++i) {
        for (unsigned j = 0; j < maxw[i]; ++j) fputc('-', f);
        fputc(' ', f);
    }
    fprintf(f, "\n");

    // Print rows
    for (unsigned i = 0; i < curr_quad; ++i) {
        // quad#
        snprintf(buf, sizeof(buf), "%u", i + 1);
        fprintf(f, "%-*s ", maxw[0], buf);

        // opcode
        snprintf(buf, sizeof(buf), "%s", iopcode_to_string(quads[i].op));
        fprintf(f, "%-*s ", maxw[1], buf);

        // result
        buf[0] = 0; expr_to_buf(quads[i].result, buf, sizeof(buf));
        fprintf(f, "%-*s ", maxw[2], buf);

        // arg1
        buf[0] = 0; expr_to_buf(quads[i].arg1, buf, sizeof(buf));
        fprintf(f, "%-*s ", maxw[3], buf);

        // arg2
        buf[0] = 0; expr_to_buf(quads[i].arg2, buf, sizeof(buf));
        fprintf(f, "%-*s ", maxw[4], buf);

        // label
if (
    quads[i].op == jump ||
    (quads[i].op >= if_eq && quads[i].op <= if_greater) // adjust range if needed for your conditionals
) {
    snprintf(buf, sizeof(buf), "%u", quads[i].label + 1);
} else {
    buf[0] = 0;
}
fprintf(f, "%-*s ", maxw[5], buf);

        // line
        snprintf(buf, sizeof(buf), "[line %u]", quads[i].line);
        fprintf(f, "%-*s", maxw[6], buf);

        fprintf(f, "\n");
    }

    fclose(f);
}


// Returns the label for the next quad (used for jumps)
unsigned nextquadlabel(void) {
    return curr_quad;
}