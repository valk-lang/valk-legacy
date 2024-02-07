
#include "../all.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->type = type;
    sc->parent = parent;
    sc->identifiers = map_make(alc);
    sc->type_identifiers = NULL;
    sc->ast = NULL;
    sc->rett = NULL;
    sc->decls = type == sc_func ? array_make(alc, 20) : NULL;
    sc->must_return = false;
    sc->did_return = false;
    return sc;
}
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent) {
    Scope* sub = scope_make(alc, type, parent);
    return sub;
}

void scope_set_idf(Scope* scope, char*name, Idf* idf, Fc* fc) {
    Build* b = fc->b;
    if(map_contains(scope->identifiers, name)) {
        sprintf(b->char_buf, "Name already taken: '%s'", name);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    map_set_force_new(scope->identifiers, name, idf);
}

void scope_add_decl(Scope* scope, Decl* decl) {
    while(scope && scope->type != sc_func) {
        scope = scope->parent;
    }
    if(!scope)
        return;
    array_push(scope->decls, decl);
}
