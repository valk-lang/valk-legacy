
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

// Read
char* tok(Parser* p, bool allow_space, bool allow_newline, bool update);
void tok_expect(Parser* p, char* expect, bool allow_space, bool allow_newline);
int tok_expect_two(Parser* p, char* expect_1, char* expect_2, bool allow_space, bool allow_newline);
void tok_skip_whitespace(Parser* p);

struct Id {
    char* ns;
    char* name;
};
struct Idf {
    int type;
    void* item;
};
struct Decl {
    Type* type;
    char *ir_var;
    char *ir_store_var;
    bool is_mut;
    bool is_gc;
    bool is_arg;
};
struct Global {
    char* name;
    char* export_name;
    Type* type;
    Value* value;
    Chunk *chunk_type;
    Chunk *chunk_value;
    Scope *declared_scope;
    bool is_shared;
    bool is_mut;
};
struct ValueAlias {
    Chunk* chunk;
};

#endif
