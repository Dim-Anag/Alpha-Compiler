#ifndef EXPR_H
#define EXPR_H

#include "symboltable.h" // Για να μπορεί να δείχνει σε Symbol αν χρειαστεί
#include "quads.h"
// Τύποι εκφράσεων (όπως στη θεωρία και το φροντιστήριο)
typedef enum {
    var_e,
    tableitem_e,
    programfunc_e,
    libraryfunc_e,
    arithexpr_e,
    boolexpr_e,
    assignexpr_e,
    newtable_e,
    constnum_e,
    constbool_e,
    conststring_e,
    nil_e,
    not_e,
    and_e,
    or_e,
    relop_e,
    member_e
} expr_t;

// Η βασική δομή για κάθε έκφραση στη γλώσσα
typedef struct expr {
    expr_t type;         // Τύπος έκφρασης (π.χ. αριθμός, μεταβλητή, bool, string, table, κλπ)
    struct Symbol* sym;  // Αν δείχνει σε μεταβλητή ή σύμβολο (π.χ. για var_e, programfunc_e)
    struct expr* index;  // Για tableitem (π.χ. t[x])
    double numConst;     // Για αριθμητικές σταθερές
    char* strConst;      // Για string σταθερές
    unsigned char boolConst; // Για boolean σταθερές
    struct quadlist* truelist;   // <-- άλλαξε εδώ
    struct quadlist* falselist;  // <-- και εδώ
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
exprlist* reverse_exprlist(exprlist* head);
struct expr* emit_iftableitem(struct expr* e);
void make_stmt(stmt_t* s);
exprlist* reverse_exprlist(exprlist* head);
struct expr* make_call(struct expr* func, struct exprlist* args);
#endif