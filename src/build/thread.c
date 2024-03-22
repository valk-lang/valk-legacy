
#include "../all.h"

Thread *thread_make(Allocator *alc, void* (fn)(void *), void *arg, Array *pool, int max_concurrent) {
#ifdef WIN32
    if (pool->length >= max_concurrent) {
        // Wait for the first thread
        Thread *t = array_pop_first(pool);
        WaitForSingleObject(t->thr, INFINITE);
    }

    void *thr = CreateThread(NULL, 0, fn, arg, 0, NULL);
#else
    if (pool->length >= max_concurrent) {
        // Wait for the first thread
        Thread *t = array_pop_first(pool);
        pthread_join(*(pthread_t *)(t->thr), NULL);
    }

    pthread_t *thr = al(alc, sizeof(pthread_t));
    pthread_create(thr, NULL, fn, arg);
#endif

    Thread* t = al(alc, sizeof(Thread));
    t->thr = thr;
    array_push(pool, t);

    return t;
}

void thread_wait_all(Array* pool) {
    for (int i = 0; i < pool->length; i++) {
        Thread *t = array_get_index(pool, i);
#ifdef WIN32
        WaitForSingleObject(t->thr, INFINITE);
#else
        pthread_join(*(pthread_t*)(t->thr), NULL);
#endif
    }
}
