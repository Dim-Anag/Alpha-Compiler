#include <stdlib.h>
#include <string.h>
#include "expr.h"
#include "quads.h"
#include "temp.h" 
#include "symboltable.h"

extern int yylineno;
extern quad* quads;
extern unsigned curr_quad;
extern unsigned total_quads;
extern SymbolTable* symbol_table;
extern unsigned int current_scope;
int debug_mode = 1; 
int is_relational(struct expr* e);
/* Δημιουργεί μια νέα έκφραση του δοσμένου τύπου    */
expr* newexpr(expr_t t) {
    expr* e = (expr*)malloc(sizeof(expr));
    if (!e) exit(1);
    memset(e, 0, sizeof(expr));
    e->type = t;
    return e;
}

/* Δημιουργεί μια νέα αριθμητική σταθερά    */
expr* newexpr_constnum(double n) {
    expr* e = newexpr(constnum_e);
    e->numConst = n;
    return e;
}

/* Δημιουργεί μια νέα string σταθερά    */
expr* newexpr_conststring(const char* s) {
    expr* e = newexpr(conststring_e);
    e->strConst = strdup(s);
    return e;
}

/* Δημιουργεί μια νέα boolean σταθερά   */
expr* newexpr_constbool(unsigned char b) {
    expr* e = newexpr(constbool_e);
    e->boolConst = b;
    return e;
}

/* Δημιουργεί μια nil έκφραση    */
expr* newexpr_nil(void) {
    return newexpr(nil_e);
}


expr* to_bool(expr* e) {
    if (debug_mode) {
      //  fprintf(stderr, "DEBUG: to_bool called at quad %u for expr type %d, op %d\n", nextquad(), e ? e->type : -1, e ? e->op : -1);
    }

    if (!e) return NULL;

    // Handle lazy NOT
    if (e->type == not_e) {
        expr* inner = to_bool(e->not_expr);

        // If inner is a temp (var_e) or a constbool_e, emit if_eq for false
        if (inner->type == var_e || inner->type == constbool_e) {
            expr* bool_e = newexpr(boolexpr_e);
            emit(if_eq, inner, newexpr_constbool(0), NULL, 0, yylineno);
            bool_e->truelist = makelist(nextquad() - 1);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
            bool_e->falselist = makelist(nextquad() - 1);
            return bool_e;
        }

        // Usual lazy logic (swap truelist/falselist)
        expr* bool_e = newexpr(boolexpr_e);
        bool_e->truelist = inner->falselist;
        bool_e->falselist = inner->truelist;
        return bool_e;
    }

    // Handle relational operators
    if (e->type == relop_e) {
        expr* left = e->left;
        expr* right = e->right;

        // Convert any lazy boolean to temp
        if (left && (
            left->type == boolexpr_e ||
            left->type == and_e ||
            left->type == or_e ||
            left->type == relop_e ||
            left->type == not_e
        )) {
            left = bool_to_temp(to_bool(left));
        }
        if (right && (
            right->type == boolexpr_e ||
            right->type == and_e ||
            right->type == or_e ||
            right->type == relop_e ||
            right->type == not_e
        )) {
            right = bool_to_temp(to_bool(right));
        }

        expr* bool_e = newexpr(boolexpr_e);

        // Emit the correct quad for each relational operator
        switch (e->op) {
            case if_eq:
                emit(if_eq, left, right, NULL, 0, yylineno);
                break;
            case if_noteq:
                emit(if_noteq, left, right, NULL, 0, yylineno);
                break;
            case if_less:
                emit(if_less, left, right, NULL, 0, yylineno);
                break;
            case if_lesseq:
                emit(if_lesseq, left, right, NULL, 0, yylineno);
                break;
            case if_greater:
                emit(if_greater, left, right, NULL, 0, yylineno);
                break;
            case if_greatereq:
                emit(if_greatereq, left, right, NULL, 0, yylineno);
                break;
            default:
                fprintf(stderr, "ERROR: Unknown relational operator in to_bool: %d\n", e->op);
                break;
        }

        bool_e->truelist = makelist(nextquad() - 1);
        emit(jump, NULL, NULL, NULL, 0, yylineno);
        bool_e->falselist = makelist(nextquad() - 1);
        return bool_e;
    }

    // Logical AND
    if (e->type == and_e) {
        expr* left = to_bool(e->left);
        backpatch(left->truelist, nextquad());
        expr* right = to_bool(e->right);
        expr* bool_e = newexpr(boolexpr_e);
        bool_e->truelist = right->truelist;
        bool_e->falselist = merge(left->falselist, right->falselist);
        return bool_e;
    }

    // Logical OR
    if (e->type == or_e) {
        expr* left = to_bool(e->left);
        unsigned after_left = nextquad();
        backpatch(left->falselist, after_left);
        expr* right = to_bool(e->right);
        expr* bool_e = newexpr(boolexpr_e);
        bool_e->truelist = merge(left->truelist, right->truelist);
        bool_e->falselist = right->falselist;
        return bool_e;
    }

    if (e->type == boolexpr_e)
        return e;

    if (e->type == constbool_e) {
        expr* bool_e = newexpr(boolexpr_e);
        emit(if_eq, newexpr_constbool(e->boolConst), newexpr_constbool(1), NULL, 0, yylineno);
        bool_e->truelist = makelist(nextquad() - 1);
        emit(jump, NULL, NULL, NULL, 0, yylineno);
        bool_e->falselist = makelist(nextquad() - 1);
        return bool_e;
    }

    // Default: treat as "is true"
    expr* bool_e = newexpr(boolexpr_e);
    emit(if_eq, e, newexpr_constbool(1), NULL, 0, yylineno);
    bool_e->truelist = makelist(nextquad() - 1);
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    bool_e->falselist = makelist(nextquad() - 1);
    return bool_e;
}


expr* bool_to_temp(expr* bool_expr) {
    if (debug_mode) {
      //  fprintf(stderr, "DEBUG: bool_to_temp at quad %u\n", nextquad());
    }
    expr* temp = newexpr(var_e);
    temp->sym = newtemp(symbol_table, current_scope);
    backpatch(bool_expr->truelist, nextquad());
    emit(assign, temp, newexpr_constbool(1), NULL, 0, yylineno);
    emit(jump, NULL, NULL, NULL, nextquad() + 2, yylineno);
    backpatch(bool_expr->falselist, nextquad());
    emit(assign, temp, newexpr_constbool(0), NULL, 0, yylineno);
    return temp;
}

quadlist* makelist(unsigned quad) {
    if (debug_mode) {
      //  fprintf(stderr, "DEBUG: makelist for quad %u\n", quad);
    }
    quadlist* l = malloc(sizeof(quadlist));
    l->quad = quad;
    l->next = NULL;
    return l;
}

quadlist* merge(quadlist* l1, quadlist* l2) {
    if (l1 == l2) return l1; // Prevent self-merge (cycle)
    if (!l1) return l2;
    quadlist* tmp = l1;
    while (tmp->next) {
        // Prevent accidental cycle if l2 is already in the list
        if (tmp->next == l2) return l1;
        tmp = tmp->next;
    }
    tmp->next = l2;
    return l1;
}

void backpatch(quadlist* list, unsigned quad) {
    while (list) {
        if (debug_mode) {
            fprintf(stderr, "DEBUG: backpatching quad %u to label %u\n", list->quad, quad);
        }
        quads[list->quad].label = quad;
        list = list->next;
    }
}

int is_relational(expr* e) {
    int res = e && e->type == boolexpr_e &&
        (e->op == if_less || e->op == if_greater || e->op == if_eq ||
         e->op == if_noteq || e->op == if_lesseq || e->op == if_greatereq);
    if (e) fprintf(stderr, "DEBUG is_relational: type=%d, op=%d, res=%d\n", e->type, e->op, res);
    return res;
}

// Δημιουργεί μια νέα έκφραση τύπου programfunc_e για user function
expr* newexpr_programfunc(Symbol* sym) {
    expr* e = newexpr(programfunc_e);
    e->sym = sym;
    return e;
}

// Δημιουργεί μια νέα έκφραση τύπου libraryfunc_e για library function
expr* newexpr_libraryfunc(Symbol* sym) {
    expr* e = newexpr(libraryfunc_e);
    e->sym = sym;
    return e;
}
    
expr* newexpr_programfunc_name(const char* name) {
    Symbol* sym = lookup_symbol(symbol_table, name, current_scope, 1);
    if (!sym) {
        sym = insert_symbol(symbol_table, name, USER_FUNCTION, yylineno, current_scope);
    }
    return newexpr_programfunc(sym);
}

expr* emit_iftableitem(expr* e) {
    if (!e) {
        fprintf(stderr, "emit_iftableitem: ERROR - null expr\n");
        return NULL;
    }
    if (e->type != tableitem_e) {
        if (!e->sym) {
            fprintf(stderr, "emit_iftableitem: WARNING - non-table item has NULL sym (type = %d)\n", e->type);
        }
        return e;
    }

    expr* result = newexpr(var_e);
    result->sym = newtemp(symbol_table, current_scope);

    expr* table = newexpr(var_e);
    table->sym = e->sym;

    emit(tablegetelem, result, table, e->index, 0, yylineno);
    return result;
}
expr* member_item(expr* lv, char* name) {
    lv = emit_iftableitem(lv);
    expr* ti = newexpr(tableitem_e);
    ti->sym = lv->sym;
    ti->index = newexpr_conststring(name);
    return ti;
}

void make_stmt(stmt_t* s) {
    s->breaklist = NULL;
    s->contlist = NULL;
}

exprlist* reverse_exprlist(exprlist* head) {
    exprlist* prev = NULL;
    exprlist* curr = head;
    while (curr) {
        exprlist* next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

expr* make_call(expr* func, exprlist* args) {
    // Count arguments
    int count = 0;
    for (exprlist* l = args; l; l = l->next) count++;
    expr** params = malloc(count * sizeof(expr*));
    int i = count - 1;
    for (exprlist* l = args; l; l = l->next, i--) params[i] = l->expr;
    for (i = count - 1; i >= 0; i--) emit(param, params[i], NULL, NULL, 0, yylineno);
    free(params);

    // printf("make_call: func type = %d, name = %s\n", func ? func->type : -1, func && func->sym ? func->sym->name : "(null)");


    //emit(call, func, NULL, NULL, 0, yylineno);
    emit(call, NULL, func, NULL, 0, yylineno);
    expr* temp = newexpr(var_e);
    temp->sym = newtemp(symbol_table, current_scope);
    emit(getretval, temp, NULL, NULL, 0, yylineno);
    return temp;
}

void patchlist(struct quadlist* list, unsigned quad) {
  //  fprintf(stderr, "DEBUG: patchlist called for quad %u\n", quad);
    while (list) {
      //  fprintf(stderr, "DEBUG: patching quad %u to label %u\n", list->quad, quad);
        quads[list->quad].label = quad;
        list = list->next;
    }
}


expr* copyexpr(expr* e) {
    if (!e) return NULL;
   // printf("copyexpr: type=%d, sym=%p, name=%s\n",
     //   e->type,
      //  (void*)e->sym,
      //  (e->sym && e->sym->name) ? e->sym->name : "(null)");
    expr* new_e = newexpr(e->type);
    new_e->sym = e->sym; // OK αν το symbol table δεν αλλάζει
    new_e->index = e->index ? copyexpr(e->index) : NULL;
    new_e->numConst = e->numConst;
    new_e->boolConst = e->boolConst;
    new_e->strConst = e->strConst ? strdup(e->strConst) : NULL;
    //
  //  printf("copyexpr (return): ptr=%p, type=%d, sym=%p, name=%s\n",
   // (void*)new_e, new_e->type, (void*)new_e->sym,
   // (new_e->sym && new_e->sym->name) ? new_e->sym->name : "(null)");
    return new_e;
    
}


