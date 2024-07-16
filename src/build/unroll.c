
#include "../all.h"

Unroll* unroll_make(Allocator* alc) {
    Unroll* u = al(alc, sizeof(Unroll));
    return u;
}
