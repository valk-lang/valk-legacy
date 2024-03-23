
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

// Read
char tok(Parser* p, bool allow_space, bool allow_newline, bool update);
void tok_expect(Parser* p, char* expect, bool allow_space, bool allow_newline);
char tok_expect_two(Parser* p, char* expect_1, char* expect_2, bool allow_space, bool allow_newline);
void tok_expect_newline(Parser* p);
bool tok_next_is_whitespace(Parser* p);

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
    char *name;
    char *ir_var;
    char *ir_store_var;
    bool is_mut;
    bool is_gc;
    bool is_arg;
};
struct DeclOverwrite {
    Decl* decl;
    Type* type;
};
struct Global {
    char* name;
    char* export_name;
    Type* type;
    Value* value;
    Chunk *chunk_type;
    Chunk *chunk_value;
    Scope *declared_scope;
    Fc* fc;
    int act;
    bool is_shared;
    bool is_mut;
};
struct ValueAlias {
    Chunk* chunk;
    Scope* scope;
    Fc* fc;
    int act;
};
struct Alias {
    char* name;
    Idf* idf;
    Chunk* chunk;
    Scope* scope;
};

#endif
