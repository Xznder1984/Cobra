#include "compiler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void print_usage(void) {
    printf("Cobra Compiler v%s\n", COBRA_VERSION);
    printf("Usage: cobrac [options] <source.cb>\n");
    printf("Options:\n");
    printf("  -o <file>    Set output file\n");
    printf("  -p <dir>     Set project directory\n");
    printf("  -O<level>    Set optimization level (0-3)\n");
    printf("  -S           Generate assembly only\n");
    printf("  -v           Verbose output\n");
    printf("  -l<lib>      Link against library\n");
    printf("  -L<dir>      Add library directory\n");
    printf("  --help       Show this help\n");
    printf("  --version    Show version\n");
}

int main(int argc, char *argv[]) {
    const char *source_path = NULL;
    const char *output_path = NULL;
    const char *project_dir = NULL;
    int optimize = 0;
    int verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            printf("Cobra Compiler v%s\n", COBRA_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
            continue;
        }
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            project_dir = argv[++i];
            continue;
        }
        if (argv[i][0] == '-' && argv[i][1] == 'O') {
            optimize = atoi(argv[i] + 2);
            continue;
        }
        if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "-L") == 0) {
            if (i + 1 < argc) {
                i++;
            }
            continue;
        }
        if (strncmp(argv[i], "-l", 2) == 0 || strncmp(argv[i], "-L", 2) == 0) {
            continue;
        }
        if (argv[i][0] != '-') {
            source_path = argv[i];
        }
    }

    if (!source_path) {
        print_usage();
        return 1;
    }

    Compiler *compiler = compiler_create();
    compiler->optimize_level = optimize;
    compiler->verbose = verbose;
    if (project_dir) {
        compiler->project_dir = strdup(project_dir);
    }

    int result = compiler_compile_file(compiler, source_path, output_path);

    if (!result && compiler->diags->count > 0) {
        diag_print_all(compiler->diags);
    }

    int err_count = 0;
    for (int i = 0; i < compiler->diags->count; i++) {
        if (compiler->diags->items[i].level == DIAG_ERROR) err_count++;
    }

    compiler_free(compiler);

    if (result) {
        if (verbose) printf("Compilation succeeded.\n");
        return 0;
    } else {
        fprintf(stderr, "Compilation failed with %d error(s).\n", err_count);
        return 1;
    }
}
