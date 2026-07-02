#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t total_allocated = 0;
static size_t peak_allocated = 0;
static size_t allocation_count = 0;

void *cobra_track_alloc(size_t size) {
    total_allocated += size;
    allocation_count++;
    if (total_allocated > peak_allocated) {
        peak_allocated = total_allocated;
    }
    return NULL;
}

void cobra_track_free(size_t size) {
    if (size <= total_allocated) {
        total_allocated -= size;
    }
}

size_t cobra_memory_used(void) {
    return total_allocated;
}

size_t cobra_memory_peak(void) {
    return peak_allocated;
}

size_t cobra_allocation_count(void) {
    return allocation_count;
}

void cobra_memory_stats(void) {
    fprintf(stderr, "Cobra Memory Stats:\n");
    fprintf(stderr, "  Current: %zu bytes\n", total_allocated);
    fprintf(stderr, "  Peak:    %zu bytes\n", peak_allocated);
    fprintf(stderr, "  Allocs:  %zu\n", allocation_count);
}
