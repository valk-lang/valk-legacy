
#ifndef _H_UNROLL
#define _H_UNROLL

#include "typedefs.h"

Unroll* unroll_make(Allocator* alc);
void unroll_scope(Unroll* ur, Scope* scope);

struct Unroll {
    Allocator* alc;
    Build* b;
    Func* func;
};

#endif