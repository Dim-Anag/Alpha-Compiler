#ifndef EXPR_H
#define EXPR_H

#include "symboltable.h" 
#include "quads.h"
// Τύποι εκφράσεων (όπως στη θεωρία και το φροντιστήριο)
typedef enum {
    var_e,          // 0
    tableitem_e,    // 1
    programfunc_e,  // 2
    libraryfunc_e,  // 3
    arithexpr_e,    // 4
    boolexpr_e,     // 5
    assignexpr_e,   // 6
    newtable_e,     // 7
    constnum_e,     // 8
    constbool_e,    // 9
    conststring_e,  // 10
    nil_e,          // 11
    not_e,          // 12
    and_e,          // 13
    or_e,           // 14
    relop_e,        // 15
    member_e        // 16
} expr_t;

// Η βασική δομή για κάθε έκφραση στη γλώσσα
typedef struct expr {
    expr_t type;         // Τύπος έκφρασης (π.χ. αριθμός, μεταβλητή, bool, string, table, κλπ)
    struct Symbol* sym;  // Αν δείχνει σε μεταβλητή ή σύμβολο (π.χ. για var_e, programfunc_e)
    struct expr* index;  // Για tableitem (π.χ. t[x])
    double numConst;     // Για αριθμητικές σταθερές
    char* strConst;      // Για string σταθερές
    unsigned char boolConst; // Για boolean σταθερές
    struct quadlist* truelist;   
    struct quadlist* falselist; 
    struct expr* next;   // Για λίστες εκφράσεων (π.χ. παραμέτρους)
    int isNot;            // 1 αν είναι delayed NOT
    struct expr* not_expr; // δείκτης στο εσωτερικό expr
    iopcode op; // Για να ξέρουμε ποια είναι η πράξη που θα κάνουμε
    struct exprlist* elist; // Για object/table constructors
    struct expr* left;
    struct expr* right;
    
} expr;

typedef struct quadlist {
    unsigned quad;
    struct quadlist* next;
} quadlist;

typedef struct stmt_t {
    struct quadlist* breaklist;
    struct quadlist* contlist;
} stmt_t;

quadlist* makelist(unsigned quad);
quadlist* merge(quadlist* l1, quadlist* l2);
void backpatch(quadlist* list, unsigned quad);
void patchlist(struct quadlist* list, unsigned quad);
typedef struct expr expr;
typedef struct exprlist {
    expr* expr;
    struct exprlist* next;
} exprlist;


// Συναρτήσεις δημιουργίας expr
expr* newexpr(expr_t t);
expr* newexpr_constnum(double n);
expr* newexpr_conststring(const char* s);
expr* newexpr_constbool(unsigned char b);
expr* newexpr_nil(void);
expr* bool_to_temp(expr* bool_expr);
expr* newexpr_programfunc(Symbol* sym);
expr* newexpr_libraryfunc(Symbol* sym);
expr* newexpr_programfunc_name(const char* name);
expr* copyexpr(expr* e);
exprlist* reverse_exprlist(exprlist* head);
struct expr* emit_iftableitem(struct expr* e);
void make_stmt(stmt_t* s);
exprlist* reverse_exprlist(exprlist* head);
struct expr* make_call(struct expr* func, struct exprlist* args);
#endif