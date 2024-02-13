
#include "../all.h"

Scope* gen_snippet_ast(Allocator* alc, Fc* fc, Snippet* snip, Map* idfs, Scope* scope_parent) {
    Scope *sub = scope_sub_make(alc, sc_default, scope_parent, NULL);
    sub->prio_idf_scope = snip->fc_scope;
    sub->identifiers = idfs;
    sub->ast = array_make(alc, 20);

    Chunk ch;
    ch = *fc->chunk_parse;
    *fc->chunk_parse = *snip->chunk;
    read_ast(fc, sub, false);
    *fc->chunk_parse = ch;

    Array* exports = snip->exports;
    if(exports) {
        for(int i = 0; i < exports->length; i++) {
            char *name = array_get_index(exports, i);
            Idf* idf = map_get(sub->identifiers, name);
            if(!idf) {
                sprintf(fc->b->char_buf, "Export variable not found in snippet: '%s'", name);
                parse_err(fc->chunk_parse, fc->b->char_buf);
            }
            scope_set_idf(scope_parent, name, idf, fc);
        }
    }

    return sub;
}
