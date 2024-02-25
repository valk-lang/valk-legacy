
#include "../all.h"

Scope* scope_make(Allocator* alc, int type, Scope* parent) {
    Scope* sc = al(alc, sizeof(Scope));
    sc->type = type;
    sc->parent = parent;
    sc->loop_scope = NULL;
    sc->prio_idf_scope = NULL;
    sc->identifiers = map_make(alc);
    sc->type_identifiers = NULL;
    sc->ast = NULL;
    sc->rett = NULL;
    sc->decls = type == sc_func ? array_make(alc, 20) : NULL;
    sc->chunk_end = NULL;
    sc->ir_after_block = NULL;
    sc->ir_cond_block = NULL;
    sc->func = NULL;
    sc->must_return = false;
    sc->did_return = false;
    return sc;
}
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent, Chunk* chunk_end) {
    Scope* sub = scope_make(alc, type, parent);
    sub->rett = parent->rett;
    sub->loop_scope = parent->loop_scope;
    sub->chunk_end = chunk_end;
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
            return;
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

Scope* scope_get_func(Scope* scope, bool must_exist, Fc* fc) {
    while(scope && scope->type != sc_func) {
        scope = scope->parent;
    }
    if(!scope && must_exist) {
        parse_err(fc->chunk_parse, "Missing function scope");
    }
    return scope;
}
