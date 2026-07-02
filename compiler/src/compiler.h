#ifndef COBRA_COMPILER_H
#define COBRA_COMPILER_H

#include "utils.h"
#include "ast.h"

typedef struct {
    char *source_path;
    char *output_path;
    char *source;
    DiagnosticList *diags;
    int optimize_level;
    int emit_ir;
    int verbose;
} Compiler;

Compiler *compiler_create(void);
void compiler_free(Compiler *c);
int compiler_compile(Compiler *c);
int compiler_compile_file(Compiler *c, const char *source_path, const char *output_path);

#endif
