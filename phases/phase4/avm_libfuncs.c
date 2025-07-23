#include "avm.h"
#include <math.h>
#include "avm_instruction.h"
#include "avm_libfuncs.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

unsigned totalNamedLibfuncs = 0;
char** namedLibfuncs = NULL;
extern unsigned totalActuals;


libfunc_entry libfuncs[] = {
    {"print", libfunc_print},
    {"input", libfunc_input},
    {"typeof", libfunc_typeof},
    {"totalarguments", libfunc_totalarguments},
    {"argument", libfunc_argument},
    {"objectmemberkeys", libfunc_objectmemberkeys},
    {"objecttotalmembers", libfunc_objecttotalmembers},
    {"objectcopy", libfunc_objectcopy},
    {"strtonum", libfunc_strtonum},
    {"sqrt", libfunc_sqrt},
    {"cos", libfunc_cos},
    {"sin", libfunc_sin},
    {NULL, NULL}
};

/* Lookup and call a library function by name   */
void avm_calllibfunc(char* id) {
  //  printf("DEBUG: avm_calllibfunc called with id=%p, '%s'\n", (void*)id, id ? id : "(null)");
    if (!id) {
        fprintf(stderr, "Runtime error: call to library function with NULL id!\n");
        executionFinished = 1;
        return;
    }
    for (int i = 0; libfuncs[i].id != NULL; ++i) {
        if (strcmp(libfuncs[i].id, id) == 0) {
            (*libfuncs[i].addr)();
            // --- cleanup actual arguments from stack ---
            if (totalActuals > 0) {
                top -= totalActuals;
                totalActuals = 0;
            }
            return;
        }
    }
    fprintf(stderr, "Runtime error: unsupported lib func '%s' called!\n", id);
    executionFinished = 1;
}


/* Print: prints all arguments  */
void libfunc_print(void) {
    unsigned n = avm_totalactuals();
    for (int i = n - 1; i >= 0; --i) { // Από το τελευταίο προς το πρώτο
        avm_memcell* m = avm_getactual(i);
        char* s = avm_tostring(m);
        printf("%s", s);
        free(s);
    }
    printf("\n");
}


/* Input: reads a line from stdin and tries to convert to number, bool, nil, or string  */
void libfunc_input(void) {
    char buffer[1024];
    avm_memcellclear(&retval);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        retval.type = nil_m;
        return;
    }
    buffer[strcspn(buffer, "\n")] = 0;

    if (buffer[0] == '"' && buffer[strlen(buffer)-1] == '"' && strlen(buffer) >= 2) {
        buffer[strlen(buffer)-1] = 0;
        retval.type = string_m;
        retval.data.strVal = strdup(buffer+1); 
        return;
    }

    /* Try to parse as number   */   
    char* endptr;
    double num = strtod(buffer, &endptr);
    if (endptr != buffer && *endptr == '\0') {
        retval.type = number_m;
        retval.data.numVal = num;
    } else if (strcmp(buffer, "true") == 0) {
        retval.type = bool_m;
        retval.data.boolVal = 1;
    } else if (strcmp(buffer, "false") == 0) {
        retval.type = bool_m;
        retval.data.boolVal = 0;
    } else if (strcmp(buffer, "nil") == 0) {
        retval.type = nil_m;
    } else {
        retval.type = string_m;
        retval.data.strVal = strdup(buffer);
    }
}

/* Typeof: returns the type of the first argument as a string   */
void libfunc_typeof(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        fprintf(stderr, "Runtime error: typeof expects 1 argument\n");
        retval.type = nil_m;
        return;
    }
    static const char* typeStrings[] = {
        "number",        
        "string",        
        "boolean",        
        "table",         
        "userfunction",  
        "libraryfunction",
        "nil",            
        "undef"         
    };
    avm_memcell* arg = avm_getactual(0);
    const char* result = typeStrings[arg->type];
  //  printf("DEBUG: typeof called with type=%d, will return \"%s\"\n", arg->type, result);
    retval.type = string_m;
    retval.data.strVal = strdup(result);
}

/* totalarguments: returns the number of actual arguments in the current function call  */
void libfunc_totalarguments(void) {
    avm_memcellclear(&retval);

    // Αν είμαστε σε global scope
    if (topsp == AVM_STACKSIZE - 1) {
        retval.type = nil_m;
        return;
    }

    unsigned num_actuals = avm_get_envvalue(topsp + AVM_NUMACTUALS_OFFSET);
    retval.type = number_m;
    retval.data.numVal = (double)num_actuals;
}

void libfunc_argument(void) {
    avm_memcellclear(&retval);
    // Πρέπει να έχεις 1 actual (το i)
    if (avm_totalactuals() != 1) {
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != number_m) {
        retval.type = nil_m;
        return;
    }
    unsigned i = (unsigned)arg0->data.numVal;

    // Είσαι σε global scope αν topsp == AVM_STACKSIZE-1
    if (topsp == AVM_STACKSIZE - 1) {
        retval.type = nil_m;
        return;
    }

    // Η i-οστή παράμετρος της τρέχουσας συνάρτησης είναι στη στοίβα:
    // stack[topsp + 4 + i]
    avm_memcell* param = &stack[topsp + 4 + i];

    // αν υπάρχει παράμετρος
   
    unsigned total_args = (unsigned)stack[topsp + 0].data.numVal;
    if (i >= total_args) {
        retval.type = nil_m;
        return;
    }

    avm_assign(&retval, param);
}

/* objectmemberkeys: returns a table with all keys of a table argument  */
void libfunc_objectmemberkeys(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);

    if (n != 1) {
        retval.type = nil_m;
        printf("DEBUG: objectmemberkeys: wrong number of arguments (%u)\n", n);
        return;
    }

    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != table_m) {
        retval.type = nil_m;
        printf("DEBUG: objectmemberkeys: argument is not a table (type=%d)\n", arg0->type);
        return;
    }

    avm_table* t = arg0->data.tableVal;
    unsigned total = avm_table_totalmembers(t);
    avm_table* keysTable = avm_tablenew();

    printf("[ ");
    if (total == 0) {
        printf("]");
        retval.type = table_m;
        retval.data.tableVal = keysTable;
        avm_tableincrefcounter(keysTable);
        printf("\nDEBUG: objectmemberkeys: table is empty\n");
        return;
    }

    avm_memcell* keys = (avm_memcell*)malloc(sizeof(avm_memcell) * total);
    if (!keys) {
        retval.type = nil_m;
        printf("DEBUG: objectmemberkeys: malloc failed\n");
        return;
    }

    unsigned found = avm_table_keys(t, keys);
    for (unsigned i = 0; i < found; ++i) {
        avm_memcell idxcell;
        idxcell.type = number_m;
        idxcell.data.numVal = i + 1;
        if (keys[i].type == string_m)
            printf("\"%s\"", keys[i].data.strVal);
        else if (keys[i].type == number_m)
            printf("%g", keys[i].data.numVal);
        else
            printf("<?>");

        if (i < found - 1)
            printf(", ");

        avm_tablesetelem(keysTable, &idxcell, &keys[i]);
        avm_memcellclear(&keys[i]);
    }
    printf(" ]\n");
    free(keys);

    retval.type = table_m;
    retval.data.tableVal = keysTable;
    avm_tableincrefcounter(keysTable);
 //   printf("DEBUG: objectmemberkeys: done\n");
}

/* objecttotalmembers: returns the number of members in a table */
void libfunc_objecttotalmembers(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
  //  printf("DEBUG: objecttotalmembers() called with n=%u\n", n);
    if (n != 1) {
        printf("objecttotalmembers() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
  //  printf("DEBUG: objecttotalmembers() arg0->type=%d\n", arg0->type);
    if (arg0->type != table_m) {
        printf("objecttotalmembers() argument must be a table\n");
        retval.type = nil_m;
        return;
    }
    unsigned total = avm_table_totalmembers(arg0->data.tableVal);
    retval.type = number_m;
    retval.data.numVal = total;
   // printf("DEBUG: objecttotalmembers() result = %u\n", total);
    printf("%.0f\n", retval.data.numVal);
}

/* objectcopy: returns a shallow copy of a table    */
void libfunc_objectcopy(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        printf("objectcopy() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != table_m) {
        printf("objectcopy() argument must be a table\n");
        retval.type = nil_m;
        return;
    }
    avm_table* orig = arg0->data.tableVal;
    avm_table* copy = avm_tablenew();

    unsigned total = avm_table_totalmembers(orig);
    if (total == 0) {
        retval.type = table_m;
        retval.data.tableVal = copy;
        avm_tableincrefcounter(copy);
        printf("DEBUG: objectcopy() created empty table at %p\n", (void*)copy);
        return;
    }

    avm_memcell* keys = (avm_memcell*)malloc(sizeof(avm_memcell) * total);
    unsigned real_total = avm_table_keys(orig, keys);

    for (unsigned i = 0; i < real_total; ++i) {
        avm_memcell keycell;
        avm_memcell valcell;
        memset(&keycell, 0, sizeof(avm_memcell));  
        memset(&valcell, 0, sizeof(avm_memcell));   
        avm_assign(&keycell, &keys[i]);
      //  printf("DEBUG: objectcopy: key[%u] type=%d, num=%f\n", i, keycell.type, keycell.data.numVal);
        avm_memcell* val = avm_tablegetelem(orig, &keycell);
        if (val) {
            avm_assign(&valcell, val);
            avm_tablesetelem(copy, &keycell, &valcell);
            avm_memcellclear(&valcell);
        }
        avm_memcellclear(&keycell);
    }
    free(keys);

    retval.type = table_m;
    retval.data.tableVal = copy;
    avm_tableincrefcounter(copy);
 //   printf("DEBUG: objectcopy() created shallow copy at %p\n", (void*)copy);
}

/* strtonum: converts a string to a number, or returns nil  */
void libfunc_strtonum(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        printf("strtonum() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != string_m) {
        printf("strtonum() argument must be a string\n");
        retval.type = nil_m;
        return;
    }
    char* endptr;
    double num = strtod(arg0->data.strVal, &endptr);
    if (endptr != arg0->data.strVal && *endptr == '\0') {
        retval.type = number_m;
        retval.data.numVal = num;
        printf("%.6f\n", num);
    } else {
        printf("nil\n"); 
        retval.type = nil_m;
    }
}

/* sqrt: returns the square root of a number, or nil if not defined */
void libfunc_sqrt(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        printf("sqrt() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);

    if (arg0->type != number_m) {
        printf("sqrt() argument must be a number\n");
        retval.type = nil_m;
        return;
    }
    if (arg0->data.numVal < 0) {
        printf("sqrt() argument must be non-negative\n");
        retval.type = nil_m;
        return;
    }
    retval.type = number_m;
    retval.data.numVal = sqrt(arg0->data.numVal);
    printf("%.6f\n", retval.data.numVal); 
}

/* cos: returns the cosine of a number (in degrees) */
void libfunc_cos(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        printf("cos() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != number_m) {
        printf("cos() argument must be a number\n");
        retval.type = nil_m;
        return;
    }
    retval.type = number_m;
    retval.data.numVal = cos(arg0->data.numVal * M_PI / 180.0);
    printf("%.6f\n", retval.data.numVal); 
}


/* sin: returns the sine of a number (in degrees)   */
void libfunc_sin(void) {
    unsigned n = avm_totalactuals();
    avm_memcellclear(&retval);
    if (n != 1) {
        printf("sin() takes exactly one argument (%u given)\n", n);
        retval.type = nil_m;
        return;
    }
    avm_memcell* arg0 = avm_getactual(0);
    if (arg0->type != number_m) {
        printf("sin() argument must be a number\n");
        retval.type = nil_m;
        return;
    }
    retval.type = number_m;
    retval.data.numVal = sin(arg0->data.numVal * M_PI / 180.0);
    printf("%.6f\n", retval.data.numVal); 
}

