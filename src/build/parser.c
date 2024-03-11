
#include "../all.h"

Parser* parser_make(Allocator* alc, Build* b) {
    Parser* p = al(alc, sizeof(Parser));
    p->b = b;
    p->unit = NULL;
    p->tkn = NULL;

    p->chunks = al(alc, 20 * sizeof(Chunk));
    p->chunk = chunk_make(alc, b, NULL);
    p->scope_end = NULL;

    p->func = NULL;
    p->class = NULL;
    p->scope = NULL;
    p->loop_scope = NULL;

    p->line = 0;
    p->col = 0;
    p->scope_end_i = 0;

    p->chunk_index = 0;

    p->in_header = false;

    return p;
}

void parser_set_chunk(Parser* p, Chunk* chunk, bool sub_chunk) {
    int ci = p->chunk_index;
    if(sub_chunk) {
        ci++;
        if(ci == 20)
            build_err(p->b, "Too much parsing recursion. Max recursive chunks allowed: 20");
    }
    p->chunks[ci] = *chunk;
    *p->chunk = *chunk;
    p->chunk_index = ci;
}

void parser_pop_chunk(Parser* p) {
    int ci = p->chunk_index - 1;
    if(ci < 0) {
        return;
    }
    *p->chunk = p->chunks[ci];
    p->chunk_index--;
}

Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope) {

    Scope *sub = scope_sub_make(alc, sc_default, p->scope);
    if (idf_scope)
        sub->idf_parent = idf_scope;

    Chunk backup = *p->chunk;
    *p->chunk = *chunk;
    Scope *scope = p->scope;
    p->scope = sub;
    Value *val = read_value(alc, p, true, 0);
    p->scope = scope;
    *p->chunk = backup;
    return val;
}
