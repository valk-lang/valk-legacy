
#include "../all.h"

Parser* parser_make(Allocator* alc, Build* b) {
    Parser* p = al(alc, sizeof(Parser));
    p->b = b;
    p->chunks = al(alc, 20 * sizeof(Chunk));
    p->chunk = chunk_make(alc, b, NULL);
    p->func = NULL;
    p->class = NULL;
    p->scope = NULL;
    p->loop_scope = NULL;

    p->data = NULL;
    p->data_i32 = 0;
    p->data_i8 = 0;
    p->has_data = false;

    p->chunk_index = 0;

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

char* parser_data_str(Parser* p) {
    if (!p->has_data)
        return NULL;
    return p->data;
}
