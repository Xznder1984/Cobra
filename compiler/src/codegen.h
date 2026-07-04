#ifndef COBRA_CODEGEN_H
#define COBRA_CODEGEN_H

#include "ast.h"
#include "ir.h"

typedef struct {
    DiagnosticList *diags;
    FILE *output;
    int label_count;
    int str_label_count;
    int stack_offset;
    int had_error;
    IRModule *ir;
    char **strings;
    int string_count;
    int string_capacity;
    char **var_names;
    int *var_offsets;
    int *var_types;
    int var_count;
    int var_capacity;
    int current_fn_var_base;
    int fn_end_label;
} CodeGenerator;

CodeGenerator *cg_create(DiagnosticList *diags);
void cg_free(CodeGenerator *cg);
int cg_generate(CodeGenerator *cg, Node *node, const char *output_path);

#endif
