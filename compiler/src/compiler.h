#ifndef COBRA_COMPILER_H
#define COBRA_COMPILER_H

#include "utils.h"
#include "ast.h"

typedef struct {
    char *source_path;
    char *output_path;
    char *source;
    char *project_dir;
    DiagnosticList *diags;
    int optimize_level;
    int emit_ir;
    int verbose;
    int use_python;
    int use_cargo;
    char **link_flags;
    int link_flag_count;
    int link_flag_capacity;
} Compiler;

Compiler *compiler_create(void);
void compiler_free(Compiler *c);
int compiler_compile(Compiler *c);
int compiler_compile_file(Compiler *c, const char *source_path, const char *output_path);
const char **compiler_get_link_flags(Compiler *c, int *count);

#endif
