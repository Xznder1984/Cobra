#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

DiagnosticList *diag_list_create(void) {
    DiagnosticList *list = calloc(1, sizeof(DiagnosticList));
    list->capacity = 64;
    list->items = calloc(list->capacity, sizeof(Diagnostic));
    list->count = 0;
    return list;
}

void diag_list_free(DiagnosticList *list) {
    if (list) {
        free(list->items);
        free(list);
    }
}

void diag_add(DiagnosticList *list, DiagLevel level, int line, int column, const char *fmt, ...) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(Diagnostic));
    }
    Diagnostic *d = &list->items[list->count++];
    d->level = level;
    d->line = line;
    d->column = column;

    va_list args;
    va_start(args, fmt);
    vsnprintf(d->message, MAX_ERROR_LEN, fmt, args);
    va_end(args);
}

static const char *diag_level_str(DiagLevel level) {
    switch (level) {
        case DIAG_ERROR:   return "error";
        case DIAG_WARNING: return "warning";
        case DIAG_INFO:    return "info";
        default:           return "unknown";
    }
}

void diag_print(Diagnostic *d) {
    const char *color_red = "\033[31m";
    const char *color_yellow = "\033[33m";
    const char *color_cyan = "\033[36m";
    const char *color_reset = "\033[0m";

    const char *color = color_red;
    if (d->level == DIAG_WARNING) color = color_yellow;
    else if (d->level == DIAG_INFO) color = color_cyan;

    fprintf(stderr, "%s%s%s: %s\n",
            color, diag_level_str(d->level), color_reset, d->message);
    if (d->line > 0) {
        fprintf(stderr, "  --> %s:%d:%d\n", color_cyan, d->line, d->column, color_reset);
    }
}

void diag_print_all(DiagnosticList *list) {
    for (int i = list->printed_up_to; i < list->count; i++) {
        diag_print(&list->items[i]);
    }
    list->printed_up_to = list->count;
}

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NULL;
    }

    char *buf = malloc(st.st_size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, st.st_size, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = malloc(len + 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

char *str_ndup(const char *s, size_t n) {
    if (!s) return NULL;
    char *d = malloc(n + 1);
    if (d) {
        memcpy(d, s, n);
        d[n] = '\0';
    }
    return d;
}
