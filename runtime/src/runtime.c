#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CobraAllocator cobra_allocator = {
    .alloc = malloc,
    .dealloc = free,
    .realloc = realloc,
    .panic = NULL,
};

void *cobra_alloc(size_t size) {
    void *ptr = cobra_allocator.alloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Cobra runtime: out of memory\n");
        exit(1);
    }
    memset(ptr, 0, size);
    return ptr;
}

void cobra_free(void *ptr) {
    cobra_allocator.dealloc(ptr);
}

void *cobra_realloc(void *ptr, size_t size) {
    void *new_ptr = cobra_allocator.realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Cobra runtime: out of memory\n");
        exit(1);
    }
    return new_ptr;
}

CobraString *cobra_string_new(const char *s) {
    CobraString *str = cobra_alloc(sizeof(CobraString));
    size_t len = s ? strlen(s) : 0;
    str->length = len;
    str->capacity = len + 1;
    str->data = cobra_alloc(str->capacity);
    if (s) memcpy(str->data, s, len);
    str->data[len] = '\0';
    return str;
}

void cobra_string_free(CobraString *s) {
    if (s) {
        cobra_free(s->data);
        cobra_free(s);
    }
}

CobraString *cobra_string_concat(CobraString *a, CobraString *b) {
    size_t new_len = a->length + b->length;
    CobraString *result = cobra_alloc(sizeof(CobraString));
    result->length = new_len;
    result->capacity = new_len + 1;
    result->data = cobra_alloc(result->capacity);
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[new_len] = '\0';
    return result;
}

CobraList *cobra_list_new(void) {
    CobraList *list = cobra_alloc(sizeof(CobraList));
    list->length = 0;
    list->capacity = 8;
    list->items = cobra_alloc(sizeof(void *) * list->capacity);
    return list;
}

void cobra_list_free(CobraList *l) {
    if (l) {
        cobra_free(l->items);
        cobra_free(l);
    }
}

void cobra_list_push(CobraList *l, void *item) {
    if (l->length >= l->capacity) {
        l->capacity *= 2;
        l->items = cobra_realloc(l->items, sizeof(void *) * l->capacity);
    }
    l->items[l->length++] = item;
}

void *cobra_list_get(CobraList *l, size_t index) {
    if (index >= l->length) {
        fprintf(stderr, "Cobra runtime: list index out of bounds (%zu >= %zu)\n",
                index, l->length);
        exit(1);
    }
    return l->items[index];
}

CobraDict *cobra_dict_new(void) {
    CobraDict *d = cobra_alloc(sizeof(CobraDict));
    d->length = 0;
    d->capacity = 16;
    d->entries = cobra_alloc(sizeof(CobraEntry) * d->capacity);
    memset(d->entries, 0, sizeof(CobraEntry) * d->capacity);
    return d;
}

void cobra_dict_free(CobraDict *d) {
    if (d) {
        for (size_t i = 0; i < d->length; i++) {
            free(d->entries[i].key);
        }
        cobra_free(d->entries);
        cobra_free(d);
    }
}

void cobra_dict_set(CobraDict *d, const char *key, void *value) {
    for (size_t i = 0; i < d->length; i++) {
        if (strcmp(d->entries[i].key, key) == 0) {
            d->entries[i].value = value;
            return;
        }
    }
    if (d->length >= d->capacity) {
        d->capacity *= 2;
        d->entries = cobra_realloc(d->entries, sizeof(CobraEntry) * d->capacity);
    }
    d->entries[d->length].key = strdup(key);
    d->entries[d->length].value = value;
    d->length++;
}

void *cobra_dict_get(CobraDict *d, const char *key) {
    for (size_t i = 0; i < d->length; i++) {
        if (strcmp(d->entries[i].key, key) == 0) {
            return d->entries[i].value;
        }
    }
    return NULL;
}

void cobra_print(const char *s) {
    printf("%s\n", s ? s : "");
}

void cobra_print_str(const char *s) {
    if (s) printf("%s", s);
}

void cobra_print_int(long long n) {
    printf("%lld", n);
}

void cobra_print_float(double f) {
    printf("%g", f);
}

void cobra_print_bool(int b) {
    printf("%s", b ? "true" : "false");
}

void cobra_print_space(void) {
    printf(" ");
}

void cobra_print_nl(void) {
    printf("\n");
}

void cobra_println(const char *s) {
    printf("%s\n", s ? s : "");
}

long long cobra_int_from_str(const char *s) {
    return s ? strtoll(s, NULL, 10) : 0;
}

double cobra_float_from_str(const char *s) {
    return s ? strtod(s, NULL) : 0.0;
}

char *cobra_str_from_int(long long n) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%lld", n);
    return strdup(buf);
}

char *cobra_str_from_float(double f) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", f);
    return strdup(buf);
}
