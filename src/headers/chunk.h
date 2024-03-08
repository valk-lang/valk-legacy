
#ifndef _H_CHUNK
#define _H_CHUNK

#include "typedefs.h"

Chunk *chunk_make(Allocator *alc, Build *b, Fc *fc);
Chunk *chunk_clone(Allocator *alc, Chunk *ch);
void chunk_set_content(Chunk *chunk, char *content, int length);
void chunk_lex(Build* b, Chunk *chunk, ChunkPos* err_pos);

struct Chunk {
    Fc *fc;
    char *tokens;
    char *content;
    int length;
    int i;
};
struct ChunkPos {
    int i;
    int content_i;
    int line;
    int col;
};

#endif
