#include "symboltable.h"

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


void insert_symbol(SymbolTable *sym_table, const char *name, SymbolType type, int line, int scope) {
    printf("DEBUG: Inserting symbol '%s' of type '%d' at line %d in scope %d\n", name, type, line, scope);

    // Check for redeclaration in the current scope
    if (lookup_symbol(sym_table, name, scope, 1)) {
        fprintf(stderr, "Error: Redeclaration of '%s' at line %d in scope %d.\n", name, line, scope);
        exit(1);
    }

    // Insert the new symbol
    unsigned int index = hash(name);
    Symbol *new_symbol = malloc(sizeof(Symbol));
    new_symbol->name = strdup(name);
    new_symbol->type = type;
    new_symbol->line = line;
    new_symbol->scope = scope;
    new_symbol->is_active = 1; // Mark as active
    new_symbol->next = sym_table->table[index]; // Insert at the head of the list
    sym_table->table[index] = new_symbol;

    printf("DEBUG: Symbol '%s' inserted and marked as active in scope %d.\n", name, scope);
}

Symbol *lookup_symbol(SymbolTable *sym_table, const char *name, int scope, int current_scope_only) {
    printf("DEBUG: Looking up symbol '%s' in scope %d (current_scope_only=%d).\n", name, scope, current_scope_only);

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
            printf("DEBUG: Checking symbol '%s' in scope %d (is_active=%d).\n", current->name, current->scope, current->is_active);
            if (strcmp(current->name, name) == 0 && current->is_active) {
                // If restricted to the current scope, ignore symbols from parent scopes
                if (current_scope_only && current->scope != scope) {
                    current = current->next;
                    continue;
                }

                // Found the symbol
                printf("DEBUG: Found symbol '%s' in scope %d.\n", name, current->scope);
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

    printf("DEBUG: Symbol '%s' not found in any active scope.\n", name);
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
    // Check if the symbol table is initialized
    if (!symbol_table) {
        fprintf(stderr, "Error: Symbol table is not initialized.\n");
        return;
    }

    // Print all symbols grouped by scope
    int max_scope = 0;

    // Find the maximum scope dynamically
    for (int i = 0; i < HASH_SIZE; i++) {
        Symbol *current = symbol_table->table[i];
        while (current) {
            if (current->scope > max_scope) {
                max_scope = current->scope;
            }
            current = current->next;
        }
    }

    // Print symbols for each scope
    for (int scope = 0; scope <= max_scope; scope++) {
        printf("---------- #SCOPE %d -------\n", scope);

        // Collect symbols for the current scope
        Symbol *symbols[HASH_SIZE];
        int count = 0;
        for (int i = 0; i < HASH_SIZE; i++) {
            Symbol *current = symbol_table->table[i];
            while (current) {
                if (current->scope == scope) {
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

        // Print sorted symbols
        for (int i = 0; i < count; i++) {
            const char *type_str;
            switch (symbols[i]->type) {
                case USER_FUNCTION:
                    type_str = "user function";
                    break;
                case VARIABLE:
                    type_str = (scope == 0) ? "global variable" : "local variable";
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