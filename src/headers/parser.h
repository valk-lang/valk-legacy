
#ifndef _H_PARSER
#define _H_PARSER

#include "typedefs.h"

Parser* parser_make(Allocator* alc, Build* b);
void parser_save_context(Parser* p);
void parser_pop_context(Parser* p, bool restore_pos);
Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope);

struct Parser {
    Build* b;
    // State
    Unit* unit;
    ParserContext* contexts;
    Chunk* chunk;
    Chunk* scope_end;
    //
    Func* func;
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
struct ParserContext {
    Chunk chunk;
    Chunk* scope_end;
    Func* func;
    Scope* scope;
    Scope* loop_scope;
    int line;
    int col;
    int scope_end_i;
    bool in_header;
};

#endif