
#include "../all.h"

Scope* scope_make(Allocator* alc, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->parent = parent;
    sc->identifiers = map_make(alc);
    return sc;
}
