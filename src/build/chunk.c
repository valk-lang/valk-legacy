
#include "../all.h"

Chunk *chunk_make(Allocator *alc, Build *b, Fc *fc) {
    Chunk *ch = al(alc, sizeof(Chunk));
    ch->b = b;
    ch->fc = fc;
    ch->alc = alc;
    ch->parent = NULL;
    ch->tokens = NULL;
    ch->content = NULL;
    ch->length = 0;
    ch->i = 0;
    ch->line = 0;
    ch->col = 0;
    return ch;
}

Chunk *chunk_clone(Allocator *alc, Chunk *ch) {
    Chunk *new = al(alc, sizeof(Chunk));
    *new = *ch;
    new->alc = alc;
    return new;
}

void chunk_set_content(Chunk *chunk, char *content, int length) {
    //
    chunk->content = content;
    chunk->length = length;
    // Lex
    chunk_lex_start(chunk);
}
