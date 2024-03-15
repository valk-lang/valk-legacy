
#include "../all.h"

Parser* parser_make(Allocator* alc, Build* b) {
    Parser* p = al(alc, sizeof(Parser));
    p->b = b;
    p->unit = NULL;
    p->tkn = NULL;

    p->contexts = al(alc, 20 * sizeof(ParserContext));
    p->chunk = chunk_make(alc, b, NULL);

    p->func = NULL;
    p->scope = NULL;
    p->loop_scope = NULL;

    p->line = 0;
    p->col = 0;
    p->scope_end_i = 0;

    p->chunk_index = 0;

    p->in_header = false;

    return p;
}

void parser_save_context(Parser* p) {
    int ci = p->chunk_index;

    ParserContext *pc = &p->contexts[ci];
    pc->chunk = *p->chunk;
    pc->func = p->func;
    pc->scope = p->scope;
    pc->loop_scope = p->loop_scope;
    pc->line = p->line;
    pc->col = p->col;
    pc->scope_end_i = p->scope_end_i;
    pc->in_header = p->in_header;

    p->chunk_index = ci + 1;
}


void parser_pop_context(Parser* p, bool restore_pos) {
    int ci = p->chunk_index - 1;
    if(ci < 0) {
        return;
    }
    ParserContext *pc = &p->contexts[ci];
    if(restore_pos)
        *p->chunk = pc->chunk;
    p->func = pc->func;
    p->scope = pc->scope;
    p->loop_scope = pc->loop_scope;
    p->line = pc->line;
    p->col = pc->col;
    p->scope_end_i = pc->scope_end_i;
    p->in_header = pc->in_header;

    p->chunk_index = ci;
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
