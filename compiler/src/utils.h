#ifndef COBRA_UTILS_H
#define COBRA_UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COBRA_VERSION "0.1.0"
#define COBRA_VERSION_MAJOR 0
#define COBRA_VERSION_MINOR 1
#define COBRA_VERSION_PATCH 0

#define MAX_ERROR_LEN 1024
#define MAX_PATH_LEN 4096

typedef enum {
    DIAG_OK = 0,
    DIAG_ERROR = 1,
    DIAG_WARNING = 2,
    DIAG_INFO = 3,
} DiagLevel;

typedef struct {
    DiagLevel level;
    int line;
    int column;
    char message[MAX_ERROR_LEN];
} Diagnostic;

typedef struct {
    Diagnostic *items;
    int count;
    int capacity;
    int printed_up_to;
} DiagnosticList;

DiagnosticList *diag_list_create(void);
void diag_list_free(DiagnosticList *list);
void diag_add(DiagnosticList *list, DiagLevel level, int line, int column, const char *fmt, ...);
void diag_print(Diagnostic *d);
void diag_print_all(DiagnosticList *list);

char *read_file(const char *path);
char *str_dup(const char *s);
char *str_ndup(const char *s, size_t n);

#endif
