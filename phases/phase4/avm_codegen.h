#ifndef AVM_CODEGEN_H
#define AVM_CODEGEN_H

#include "avm_instruction.h"
#include "avm_const_tables.h"
#include "quads.h"

/* Generates the final VM code and writes it to the given file. */
void generate_final_code(const char* filename);

/* Prints all generated instructions to the given file. */
void print_instructions(const char* filename);

#endif /* AVM_CODEGEN_H */