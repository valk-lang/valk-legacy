
#ifndef H_CORO
#define H_CORO

#include "typedefs.h"

struct VAwait {
    Decl* decl;
    Value* on;
    void* block;
    Type* rett;
    int suspend_index;
    bool on_decl;
};

#endif
