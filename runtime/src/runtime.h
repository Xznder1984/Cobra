#ifndef COBRA_RUNTIME_H
#define COBRA_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} CobraString;

typedef struct {
    void **items;
    size_t length;
    size_t capacity;
} CobraList;

typedef struct {
    char *key;
    void *value;
} CobraEntry;

typedef struct {
    CobraEntry *entries;
    size_t length;
    size_t capacity;
} CobraDict;

CobraString *cobra_string_new(const char *s);
void cobra_string_free(CobraString *s);
CobraString *cobra_string_concat(CobraString *a, CobraString *b);

CobraList *cobra_list_new(void);
void cobra_list_free(CobraList *l);
void cobra_list_push(CobraList *l, void *item);
void *cobra_list_get(CobraList *l, size_t index);

CobraDict *cobra_dict_new(void);
void cobra_dict_free(CobraDict *d);
void cobra_dict_set(CobraDict *d, const char *key, void *value);
void *cobra_dict_get(CobraDict *d, const char *key);

void cobra_print(const char *s);
void cobra_print_int(long long n);
void cobra_print_float(double f);
void cobra_println(const char *s);

void *cobra_alloc(size_t size);
void cobra_free(void *ptr);
void *cobra_realloc(void *ptr, size_t size);

long long cobra_int_from_str(const char *s);
double cobra_float_from_str(const char *s);
char *cobra_str_from_int(long long n);
char *cobra_str_from_float(double f);

typedef struct {
    void *(*alloc)(size_t);
    void (*dealloc)(void *);
    void *(*realloc)(void *, size_t);
    void (*panic)(const char *);
} CobraAllocator;

extern CobraAllocator cobra_allocator;

#endif
