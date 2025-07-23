#include <string.h>
#include <stdlib.h>
#include "vartempmap.h"
#include "temp.h"


extern int yylineno;
// Structure for mapping variable names to temporary expressions
typedef struct {
    char* name;
    struct expr* temp;
} TempMapEntry;

#define MAX_VARS 256
static TempMapEntry temp_map[MAX_VARS];
static int temp_map_size = 0;
// Initializes the variable-to-temp map (clears all entries)
void vartempmap_init(void) {
    temp_map_size = 0;
}
// Clears the variable-to-temp map and frees memory for names
void vartempmap_clear(void) {
    for (int i = 0; i < temp_map_size; ++i) {
        free(temp_map[i].name);
        
    }
    temp_map_size = 0;
}
// Returns the temp expr for a variable name, or creates one if it doesn't exist
struct expr* get_or_create_temp(const char* varname) {
    for (int i = 0; i < temp_map_size; ++i) {
        if (strcmp(temp_map[i].name, varname) == 0)
            return temp_map[i].temp;
    }
        // Not found: create new temp expr and mapping
    struct expr* temp = newexpr(var_e);
    temp->sym = newtemp();
    temp_map[temp_map_size].name = strdup(varname);
    temp_map[temp_map_size].temp = temp;
    temp_map_size++;
    return temp;
}

// Returns 1 if the expr is a temp (name starts with '^' or '_'), 0 otherwise
int istempexpr(struct expr* e) {
    return e && e->sym && e->sym->name &&
           (e->sym->name[0] == '^' || e->sym->name[0] == '_');
}
