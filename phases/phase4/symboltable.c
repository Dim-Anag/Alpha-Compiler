#include "symboltable.h"
#include <assert.h>
int global_var_offset = 0;
int max_global_offset = -1;
int local_var_offset = 0;
int temp_var_offset = 0;
#define MAX_SYMBOLS_PER_SCOPE 1024

// Hash function to map symbol names to indices
unsigned int hash(const char *name) {
    unsigned int hash = 0;
    while (*name) {
        hash = (hash << 5) + *name++; // Shift left and add the ASCII value of the character
    }
    return hash % HASH_SIZE;
}

// Create a new symbol table
SymbolTable *create_symbol_table() {
    SymbolTable *sym_table = malloc(sizeof(SymbolTable));
    for (int i = 0; i < HASH_SIZE; i++) {
        sym_table->table[i] = NULL;
    }
    return sym_table;
}


// Destroy the symbol table
void destroy_symbol_table(SymbolTable *sym_table) {
    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol *current = sym_table->table[i];
        while (current) {
            Symbol *temp = current;
            current = current->next;
            free(temp->name);
            // free(temp->scope); // Removed: scope is an int, not a pointer
            free(temp);
        }
    }
    free(sym_table);
}


Symbol* insert_symbol(SymbolTable *sym_table, const char *name, SymbolType type, int line, int scope) {

    // Prevent shadowing of library functions in global scope
    Symbol *lib_conflict = lookup_symbol(sym_table, name, 0, 1); // Only global scope
    if (lib_conflict && lib_conflict->type == LIBRARY_FUNCTION) {
        fprintf(stderr, "Error: Cannot shadow library function '%s' at line %d.\n", name, line);
        exit(1);
    }

    // Check for redeclaration in the current scope
    if (type != TEMPVAR && lookup_symbol(sym_table, name, scope, 1)) {
        fprintf(stderr, "Error: Redeclaration of '%s' at line %d in scope %d.\n", name, line, scope);
        exit(1);
    }

    unsigned int index = hash(name);

    Symbol *new_symbol = malloc(sizeof(Symbol));
    memset(new_symbol, 0, sizeof(Symbol));
  //  printf("DEBUG: insert_symbol name=%s, type=%d, addr=%p\n", name, type, (void*)new_symbol);
    new_symbol->name = strdup(name);
    new_symbol->type = type;
    new_symbol->line = line;
    new_symbol->scope = scope;
    new_symbol->is_active = 1;
    new_symbol->next = sym_table->table[index];
    new_symbol->total_locals = 0;

    // --- ΕΝΙΑΙΟ offset για κανονικές και temps ---
if (scope == 0) { 
    extern int global_var_offset;
    new_symbol->space = programvar;
    new_symbol->offset = global_var_offset++;
    extern int max_global_offset;
    if (new_symbol->offset > max_global_offset)
        max_global_offset = new_symbol->offset;
    // if (type == TEMPVAR)
    //     printf("DEBUG: insert_symbol temporary name=%s, offset=%d, space=%d\n", name, new_symbol->offset, new_symbol->space);
} else { // function scope
    new_symbol->space = functionlocal;
    extern int local_var_offset;
    new_symbol->offset = local_var_offset++;
    // if (type == TEMPVAR)
    //    printf("DEBUG: insert_symbol temporary name=%s, offset=%d, space=%d\n", name, new_symbol->offset, new_symbol->space);
}

    sym_table->table[index] = new_symbol;

    return new_symbol;
}


Symbol *lookup_symbol(SymbolTable *sym_table, const char *name, int scope, int current_scope_only) {
  //  printf("DEBUG: Looking up symbol '%s' in scope %d (current_scope_only=%d).\n", name, scope, current_scope_only);
    // Check for global resolution (::name)
    if (name[0] == ':' && name[1] == ':') {
        // Skip the '::' prefix and search in Scope 0
        name += 2; // Move past the '::'
        scope = 0; // Restrict search to global scope
        current_scope_only = 1;
    }

    // Check the current scope and optionally parent scopes
    while (scope >= 0) {
        unsigned int index = hash(name);
        Symbol *current = sym_table->table[index];
        while (current) {
           // printf("DEBUG: Checking symbol '%s' in scope %d (is_active=%d).\n", current->name, current->scope, current->is_active);
            if (strcmp(current->name, name) == 0 && current->is_active) {
                // If restricted to the current scope, ignore symbols from parent scopes
                if (current_scope_only && current->scope != scope) {
                    current = current->next;
                    continue;
                }

                // Found the symbol
             //   printf("DEBUG: Found symbol '%s' in scope %d.\n", name, current->scope);
                return current;
            }
            current = current->next;
        }

        // If restricted to the current scope, stop searching after the first iteration
        if (current_scope_only) {
            break;
        }

        scope--; // Check parent scope
    }

  //  printf("DEBUG: Symbol '%s' not found in any active scope.\n", name);
    return NULL; // Not found
}

// Remove a symbol from the symbol table
void remove_symbol(SymbolTable *sym_table, const char *name) {
    unsigned int index = hash(name);
    Symbol *current = sym_table->table[index];
    Symbol *prev = NULL;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                sym_table->table[index] = current->next;
            }
            free(current->name);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}


void print_symbol_table(SymbolTable *symbol_table) {
    if (!symbol_table) {
        fprintf(stderr, "Error: Symbol table is not initialized.\n");
        return;
    }

    int max_scope = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol *current = symbol_table->table[i];
        while (current) {
            if (current->scope > max_scope) {
                max_scope = current->scope;
            }
            current = current->next;
        }
    }

    for (int scope = 0; scope <= max_scope; scope++) {
        printf("---------- #SCOPE %d -------\n", scope);

        Symbol *symbols[MAX_SYMBOLS_PER_SCOPE];
        int count = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Symbol *current = symbol_table->table[i];
            while (current) {
                if (current->scope == scope) {
                    if (count >= MAX_SYMBOLS_PER_SCOPE) {
                        fprintf(stderr, "Too many symbols in scope %d!\n", scope);
                        break;
                    }
                    symbols[count++] = current;
                }
                current = current->next;
            }
        }

        // Sort symbols by line number
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (symbols[i]->line > symbols[j]->line) {
                    Symbol *temp = symbols[i];
                    symbols[i] = symbols[j];
                    symbols[j] = temp;
                }
            }
        }

        for (int i = 0; i < count; i++) {
            const char *type_str;
         //   printf("DEBUG: symbol '%s' raw type = %d, offset = %d\n", symbols[i]->name, symbols[i]->type, symbols[i]->offset);
          //  printf("DEBUG: print symbol '%s' at %p, type=%d\n", symbols[i]->name, (void*)symbols[i], symbols[i]->type);
            switch (symbols[i]->type) {
                case USER_FUNCTION:
                    type_str = "user function";
                    break;
                case VARIABLE:
                    type_str = (scope == 0) ? "global variable" : "local variable";
                    break;
                case TEMPVAR:
                    type_str = (scope == 0) ? "temporary (global)" : "temporary (local)";
                    break;
                case FORMAL_ARGUMENT:
                    type_str = "formal argument";
                    break;
                case LIBRARY_FUNCTION:
                    type_str = "library function";
                    break;
                default:
                    type_str = "unknown";
                    break;
            }

            printf("\"%s\" [%s] (line %d)(scope %d)\n",
                   symbols[i]->name, type_str, symbols[i]->line, symbols[i]->scope);
        }
    }
}

// Get all symbols (optional)
void get_all_symbols(SymbolTable *sym_table, Symbol **symbols, int *count) {
    *count = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol *current = sym_table->table[i];
        while (current) {
            symbols[(*count)++] = current;
            current = current->next;
        }
    }
}


void deactivate_scope(SymbolTable *sym_table, int scope) {
  //  printf("DEBUG: Deactivating all symbols in scope %d.\n", scope); // Add debug output
    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol *current = sym_table->table[i];
        while (current) {
            if (current->scope == scope) {
                current->is_active = 0; // Mark as inactive
            //    printf("DEBUG: Deactivated symbol '%s' in scope %d.\n", current->name, scope);
            }
            current = current->next;
        }
    }
}

void initialize_library_functions(SymbolTable *sym_table) {
    insert_symbol(sym_table, "print", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "input", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "objectmemberkeys", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "objecttotalmembers", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "objectcopy", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "totalarguments", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "argument", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "typeof", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "strtonum", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "sqrt", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "cos", LIBRARY_FUNCTION, 0, 0);
    insert_symbol(sym_table, "sin", LIBRARY_FUNCTION, 0, 0);
}


void print_all_symbol_offsets(SymbolTable* sym_table) {
    printf("---- SYMBOL TABLE OFFSETS ----\n");
    for (int i = 0; i < HASH_SIZE; ++i) {
        Symbol* sym = sym_table->table[i];
        while (sym) {
            printf("name=%s, type=%d, offset=%d, scope=%d\n",
                sym->name, sym->type, sym->offset, sym->scope);
            sym = sym->next;
        }
    }
    printf("------------------------------\n");
}