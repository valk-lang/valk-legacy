
#include "../all.h"

Scope* scope_make(Allocator* alc, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->parent = parent;
    sc->identifiers = map_make(alc);
    sc->ast = NULL;
    sc->did_return = false;
    sc->rett = NULL;
    return sc;
}

void scope_set_idf(Scope* scope, char*name, Idf* idf, Fc* fc) {
    Build* b = fc->b;
    if(map_contains(scope->identifiers, name)) {
        sprintf(b->char_buf, "Name already taken: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    map_set(scope->identifiers, name, idf);
}
