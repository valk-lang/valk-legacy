
#include "../all.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->type = type;
    sc->parent = parent;
    sc->idf_parent = parent;

    sc->identifiers = map_make(alc);
    sc->type_identifiers = NULL;

    sc->ast = NULL;
    sc->decls = type == sc_func ? array_make(alc, 20) : NULL;

    sc->gc_decl_count = 0;

    sc->rett = NULL;
    sc->must_return = false;
    sc->did_return = false;
    sc->gc_check = false;
    sc->has_gc_decls = false;

    return sc;
}
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent) {
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
    Scope* closest = scope;
    while (closest && closest->type == sc_default) {
        closest = closest->parent;
    }
    while (scope && scope->type != sc_func) {
        scope = scope->parent;
    }
    if(!scope) {
        printf("Declaring a variable outside a function (compiler bug)\n");
        exit(1);
    }
    // Closest scope, e.g. if / while
    if(decl->is_gc && closest != scope) {
        if (!closest->decls)
            closest->decls = array_make(alc, 4);
        array_push(closest->decls, decl);
        closest->has_gc_decls = true;
        closest->gc_decl_count++;
    }
    // Func scope
    array_push(scope->decls, decl);
    if(decl->is_gc) {
        scope->has_gc_decls = true;
        scope->gc_decl_count++;
    }
}
