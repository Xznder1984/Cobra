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
} CodeGenerator;

CodeGenerator *cg_create(DiagnosticList *diags);
void cg_free(CodeGenerator *cg);
int cg_generate(CodeGenerator *cg, Node *node, const char *output_path);

#endif
