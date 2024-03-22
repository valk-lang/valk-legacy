
#ifndef H_THREAD
#define H_THREAD

#include "typedefs.h"

Thread *thread_make(Allocator *alc, void* (fn)(void *), void *arg, Array *pool, int max_concurrent);
void thread_wait_all(Array* pool);

struct Thread {
    void* thr;
};

#endif
