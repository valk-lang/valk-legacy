
#include "../all.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->type = type;
    sc->parent = parent;

    sc->identifiers = map_make(alc);
    sc->type_identifiers = NULL;

    sc->ast = NULL;
    sc->decls = type == sc_func ? array_make(alc, 20) : NULL;

    sc->rett = NULL;
    sc->must_return = false;
    sc->did_return = false;

    return sc;
}
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent, Chunk* chunk_end) {
    Scope* sub = scope_make(alc, type, parent);
    sub->rett = parent->rett;
    return sub;
}

void scope_set_idf(Scope* scope, char*name, Idf* idf, Parser* p) {
    if(map_contains(scope->identifiers, name)) {
        parse_err(p, -1, "Name already taken: '%s'", name);
    }
    map_set_force_new(scope->identifiers, name, idf);
}

void scope_add_decl(Allocator* alc, Scope* scope, Decl* decl) {
    if(decl->is_gc) {
        Scope *loop = scope;
        while (loop && loop->type != sc_loop) {
            loop = loop->parent;
        }
        if (loop) {
            Array* decls = loop->decls;
            if(!decls) {
                decls = array_make(alc, 4);
                loop->decls = decls;
            }
            array_push(decls, decl);
        }
    }
    while(scope && scope->type != sc_func) {
        scope = scope->parent;
    }
    if(!scope) {
        printf("Missing function scope (compiler bug)\n");
        exit(1);
    }
    array_push(scope->decls, decl);
}

