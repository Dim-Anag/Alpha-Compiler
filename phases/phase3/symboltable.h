#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef enum {
    VARIABLE,
    FORMAL_ARGUMENT, 
    USER_FUNCTION,
    LIBRARY_FUNCTION,
    MEMBER,
    LIBRARY_FUNCTION_CALL,
    USER_FUNCTION_CALL,
    BLOCK
} SymbolType;
typedef struct Symbol {
    char *name;          // Name of the symbol
    SymbolType type;     // Type of the symbol (VARIABLE, USER_FUNCTION, LIBRARY_FUNCTION)
    int line;            // Line of definition
    int scope;           // Scope of the symbol
    int is_active;       // Whether the symbol is active (1 = active, 0 = inactive)
    struct Symbol *next; // Pointer for handling collisions in the hash table
    int total_locals;
    int jump_quad;
} Symbol;

// Define the structure for the symbol table
#define HASH_SIZE 211 // A prime number for better hash distribution
typedef struct SymbolTable {
    Symbol *table[HASH_SIZE]; // Array of pointers to Symbol (chaining for collisions)
} SymbolTable;

// Function declarations
SymbolTable *create_symbol_table();
void destroy_symbol_table(SymbolTable *sym_table);
Symbol* insert_symbol(SymbolTable *sym_table, const char *name, SymbolType type, int line, int scope);
Symbol *lookup_symbol(SymbolTable *sym_table, const char *name, int scope, int current_scope_only);
void remove_symbol(SymbolTable *sym_table, const char *name);
void print_symbol_table(SymbolTable *sym_table);
void get_all_symbols(SymbolTable *sym_table, Symbol **symbols, int *count); // Optional
void initialize_library_functions(SymbolTable *sym_table);
void deactivate_scope(SymbolTable *sym_table, int scope);
#endif // SYMBOLTABLE_H