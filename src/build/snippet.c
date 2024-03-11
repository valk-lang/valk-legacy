
#include "../all.h"

Scope* gen_snippet_ast(Allocator* alc, Parser* p, Snippet* snip, Map* idfs, Scope* scope_parent) {
    Scope *sub = scope_sub_make(alc, sc_default, scope_parent);
    sub->idf_parent = snip->fc_scope;
    sub->identifiers = idfs;
    sub->ast = array_make(alc, 20);

    Map *pidfs = scope_parent->identifiers;
    for (int i = 0; i < pidfs->values->length; i++) {
        char *key = array_get_index(pidfs->keys, i);
        if (!map_contains(idfs, key)) {
            map_set(idfs, key, array_get_index(pidfs->values, i));
        }
    }

    Scope* scope = p->scope;
    Chunk ch = *p->chunk;
    *p->chunk = *snip->chunk;
    p->scope = sub;
    read_ast(p, false);
    *p->chunk = ch;
    p->scope = scope;

    Array* exports = snip->exports;
    if(exports && scope_parent) {
        for(int i = 0; i < exports->length; i++) {
            char *name = array_get_index(exports, i);
            Idf* idf = map_get(sub->identifiers, name);
            if(!idf) {
                parse_err(p, -1, "Export variable not found in snippet: '%s'", name);
            }
            scope_set_idf(scope_parent, name, idf, p);
        }
    }

    return sub;
}
