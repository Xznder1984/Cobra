#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "typechecker.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

Compiler *compiler_create(void) {
    Compiler *c = calloc(1, sizeof(Compiler));
    c->source_path = NULL;
    c->output_path = NULL;
    c->source = NULL;
    c->diags = diag_list_create();
    c->optimize_level = 0;
    c->emit_ir = 0;
    c->verbose = 0;
    c->link_flags = NULL;
    c->link_flag_count = 0;
    c->link_flag_capacity = 0;
    return c;
}

void compiler_free(Compiler *c) {
    if (c) {
        free(c->source_path);
        free(c->output_path);
        free(c->source);
        for (int i = 0; i < c->link_flag_count; i++) free(c->link_flags[i]);
        free(c->link_flags);
        diag_list_free(c->diags);
        free(c);
    }
}

static char *change_extension(const char *path, const char *new_ext) {
    const char *dot = strrchr(path, '.');
    size_t base_len = dot ? (size_t)(dot - path) : strlen(path);
    size_t ext_len = strlen(new_ext);
    char *result = malloc(base_len + ext_len + 1);
    strncpy(result, path, base_len);
    result[base_len] = '\0';
    strcat(result, new_ext);
    return result;
}

int compiler_compile_file(Compiler *c, const char *source_path, const char *output_path) {
    c->source_path = str_dup(source_path);
    c->source = read_file(source_path);

    if (!c->source) {
        diag_add(c->diags, DIAG_ERROR, 0, 0,
                 "Could not read source file '%s'", source_path);
        return 0;
    }

    if (!output_path) {
        c->output_path = change_extension(source_path, ".s");
    } else {
        c->output_path = str_dup(output_path);
    }

    if (c->verbose) {
        printf("Compiling: %s\n", source_path);
        printf("Output: %s\n", c->output_path);
    }

    return compiler_compile(c);
}

int compiler_compile(Compiler *c) {
    if (!c->source) {
        diag_add(c->diags, DIAG_ERROR, 0, 0, "No source to compile");
        return 0;
    }

    // Phase 1: Lexing
    if (c->verbose) printf("Phase 1/5: Lexing...\n");
    Lexer *lexer = lexer_create(c->source, c->diags);
    if (c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    // Phase 2: Parsing
    if (c->verbose) printf("Phase 2/5: Parsing...\n");
    Parser *parser = parser_create(lexer, c->diags);
    Node *ast = parser_parse(parser);
    if (parser->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    // Phase 3: Semantic Analysis
    if (c->verbose) printf("Phase 3/5: Semantic Analysis...\n");
    SemanticAnalyzer *semantic = semantic_create(c->diags);
    semantic_analyze(semantic, ast);
    if (semantic->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    // Phase 4: Type Checking
    if (c->verbose) printf("Phase 4/5: Type Checking...\n");
    TypeChecker *tc = tc_create(c->diags);
    tc_check(tc, ast);
    if (tc->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    // Phase 5: Code Generation
    if (c->verbose) printf("Phase 5/5: Code Generation...\n");
    CodeGenerator *cg = cg_create(c->diags);
    int result = cg_generate(cg, ast, c->output_path);
    if (!result || cg->had_error || c->diags->count > 0) {
        diag_print_all(c->diags);
    }

    // Cleanup
    cg_free(cg);
    tc_free(tc);
    semantic_free(semantic);
    node_free(ast);
    parser_free(parser);
    lexer_free(lexer);

    if (c->verbose && result) {
        printf("Compilation successful.\n");
    }

    return result;
}
