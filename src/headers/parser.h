
#ifndef _H_PARSER
#define _H_PARSER

#include "typedefs.h"

Parser* parser_make(Allocator* alc, Build* b);
void parser_set_chunk(Parser* p, Chunk* chunk, bool sub_chunk);
void parser_pop_chunk(Parser* p);

struct Parser {
    Build* b;
    Chunk* chunks;
    Chunk* chunk;
    //
    Func* func;
    Class* class;
    //
    Scope* scope;
    Scope* loop_scope;
    //
    char* tkn;
    //
    int line;
    int col;
    int scope_end_i;
    //
    int chunk_index;
    bool in_header;
};

#endif