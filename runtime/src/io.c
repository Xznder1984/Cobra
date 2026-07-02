#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    FILE *file;
    char *path;
    int is_open;
} CobraFile;

CobraFile *cobra_file_open(const char *path, const char *mode) {
    CobraFile *f = malloc(sizeof(CobraFile));
    if (!f) return NULL;
    f->file = fopen(path, mode);
    f->path = strdup(path);
    f->is_open = (f->file != NULL);
    if (!f->file) {
        free(f->path);
        free(f);
        return NULL;
    }
    return f;
}

void cobra_file_close(CobraFile *f) {
    if (f && f->file) {
        fclose(f->file);
        f->is_open = 0;
        free(f->path);
        free(f);
    }
}

char *cobra_file_read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, size, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

int cobra_file_write(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return (int)written;
}

int cobra_file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}
