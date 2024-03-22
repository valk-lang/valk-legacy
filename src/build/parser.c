
#include "../all.h"

Parser* parser_make(Allocator* alc, Unit* u) {
    Parser* p = al(alc, sizeof(Parser));
    p->b = u->b;
    p->unit = u;
    p->tkn = NULL;

    p->prev = NULL;
    p->chunk = chunk_make(alc, u->b, NULL);

    p->func = NULL;
    p->scope = NULL;
    p->loop_scope = NULL;
    p->vscope_values = NULL;

    p->line = 0;
    p->col = 0;
    p->scope_end_i = 0;
    p->cc_index = 0;

    p->in_header = false;
    p->on_newline = false;

    return p;
}

void parser_new_context(Parser** ref) {
    Parser* p = *ref;
    Parser *p2 = pool_get_parser(p->unit);

    // Copy everything except chunk
    Chunk* ch = p2->chunk;
    *p2 = *p;
    p2->chunk = ch;
    p2->prev = p;

    // Set variable
    *ref = p2;
}
void parser_pop_context(Parser** ref) {
    Parser* p2 = *ref;
    Parser* p = p2->prev;
    *ref = p;
}

Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope) {

    Scope *sub = scope_sub_make(alc, sc_default, p->scope);
    if (idf_scope)
        sub->idf_parent = idf_scope;

    Parser* p2 = pool_get_parser(p->unit);

    *p2->chunk = *chunk;
    p2->scope = sub;
    Value *val = read_value(alc, p2, true, 0);

    pool_return_parser(p->unit, p2);

    return val;
}
