
#ifndef H_CORO
#define H_CORO

#include "typedefs.h"

Value* coro_generate(Allocator* alc, Parser* p, Value* on);
Value* coro_await(Allocator* alc, Parser* p, Value* on);

struct VAwait {
    Decl* decl;
    Value* on;
    void* block;
    Type* rett;
    bool on_decl;
};

#endif
