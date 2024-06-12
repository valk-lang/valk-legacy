
#ifndef _H_PARSER
#define _H_PARSER

#include "typedefs.h"

Parser* parser_make(Allocator* alc, Unit* u);
void parser_new_context(Parser** ref);
void parser_pop_context(Parser** ref);
Value *read_value_from_other_chunk(Parser *p, Allocator* alc, Chunk *chunk, Scope *idf_scope, Type* type_check);
void read_access_type(Parser *p, char t, char* res);

struct Parser {
    Build* b;
    // State
    Unit* unit;
    Parser* prev;
    Chunk* chunk;
    //
    Func* func;
    //
    Scope* scope;
    Scope* loop_scope;
    Array* vscope_values;
    Array* cc_loops;
    //
    char cc_results[100];
    //
    char* tkn;
    //
    int line;
    int col;
    int scope_end_i;
    int cc_index;
    int cc_loop_index;
    //
    bool in_header;
    bool on_newline;
    bool parse_last;
    bool reading_coro_fcall;
    bool init_thread;
    bool allow_multi_type;
};

#endif