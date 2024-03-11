
#include "../all.h"

Chunk *chunk_make(Allocator *alc, Build *b, Fc *fc) {
    Chunk *ch = al(alc, sizeof(Chunk));
    ch->fc = fc;
    ch->tokens = NULL;
    ch->content = NULL;
    ch->length = 0;
    ch->i = 0;
    return ch;
}

Chunk *chunk_clone(Allocator *alc, Chunk *ch) {
    Chunk *new = al(alc, sizeof(Chunk));
    *new = *ch;
    return new;
}

void chunk_set_content(Build* b, Chunk *chunk, char *content, int length) {
    //
    chunk->content = content;
    chunk->length = length;
    // Lex
    chunk_lex(b, chunk, NULL);
}

ChunkPos* chunk_token_pos(Build* b, Chunk *chunk, int token_i) {
    ChunkPos* pos = al(b->alc, sizeof(ChunkPos));
    pos->i = token_i;
    pos->content_i = 0;
    pos->line = 0;
    pos->col = 0;
    chunk_lex(b, chunk, pos);
    return pos;
}
