
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

// Scopes
Scope* scope_make(Allocator* alc, int type, Scope* parent);
void scope_set_idf(Scope* scope, char*name, Idf* idf, Fc* fc);
void scope_add_decl(Allocator* alc, Scope* scope, Decl* decl);
Scope* scope_sub_make(Allocator* alc, int type, Scope* parent, Chunk* chunk_end);
Scope* scope_get_func(Scope* scope, bool must_exist, Fc* fc);
// Read
char* tok(Fc* fc, bool allow_space, bool allow_newline, bool update);
void tok_back(Fc* fc);
void tok_expect(Fc* fc, char* expect, bool allow_space, bool allow_newline);
char* tok_expect_two(Fc* fc, char* expect_1, char* expect_2, bool allow_space, bool allow_newline);
char tok_id_next(Fc* fc);
char tok_id_next_ignore_spacing(Fc* fc);
char tok_read_byte(Fc* fc, int offset);
void tok_skip_whitespace(Fc* fc);
void tok_skip_space(Fc* fc);
bool tok_next_is_whitespace(Fc* fc);
char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool read_only);
char* chunk_read(Chunk* chunk, int *i_ref);

struct Scope {
    Scope* parent;
    Scope* loop_scope;
    Scope* prio_idf_scope;
    Map* identifiers;
    Map* type_identifiers;
    Array* ast;
    Type* rett;
    Array* decls;
    int type;
    Chunk* chunk_end;
    IRBlock* ir_after_block;
    IRBlock* ir_cond_block;
    Func* func;
    bool must_return;
    bool did_return;
    bool calls_other_functions;
};
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
    bool is_shared;
    bool is_mut;
};
struct ValueAlias {
    Chunk* chunk;
    Fc* fc;
};

#endif
