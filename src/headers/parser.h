
#ifndef _H_PARSER
#define _H_PARSER

#include "typedefs.h"

Parser* parser_make(Allocator* alc, Build* b);
void parser_set_chunk(Parser* p, Chunk* chunk, bool sub_chunk);
void parser_pop_chunk(Parser* p);
char* parser_data_str(Parser* p);

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
    void* data;
    int data_i32;
    char data_i8;
    //
    int chunk_index;
    bool has_data;
};

#endif