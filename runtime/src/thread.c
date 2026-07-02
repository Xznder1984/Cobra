#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <pthread.h>
#else
#include <pthread.h>
#endif

typedef struct {
    pthread_t thread;
    void *(*func)(void *);
    void *arg;
    void *result;
} CobraThread;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
} CobraCond;

CobraThread *cobra_thread_create(void *(*func)(void *), void *arg) {
    CobraThread *t = cobra_alloc(sizeof(CobraThread));
    t->func = func;
    t->arg = arg;
    t->result = NULL;

    if (pthread_create(&t->thread, NULL, func, arg) != 0) {
        fprintf(stderr, "Cobra runtime: failed to create thread\n");
        cobra_free(t);
        return NULL;
    }
    return t;
}

void cobra_thread_join(CobraThread *t) {
    if (t) {
        pthread_join(t->thread, &t->result);
    }
}

void cobra_thread_detach(CobraThread *t) {
    if (t) {
        pthread_detach(t->thread);
    }
}

void *cobra_thread_result(CobraThread *t) {
    return t ? t->result : NULL;
}

pthread_mutex_t *cobra_mutex_create(void) {
    pthread_mutex_t *m = cobra_alloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);
    return m;
}

void cobra_mutex_lock(pthread_mutex_t *m) {
    pthread_mutex_lock(m);
}

void cobra_mutex_unlock(pthread_mutex_t *m) {
    pthread_mutex_unlock(m);
}

void cobra_mutex_free(pthread_mutex_t *m) {
    if (m) {
        pthread_mutex_destroy(m);
        cobra_free(m);
    }
}
