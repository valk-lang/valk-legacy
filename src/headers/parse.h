
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

// Scopes
Scope* scope_make(Allocator* alc, Scope* parent);
void scope_set_idf(Scope* scope, char*name, Idf* idf, Fc* fc);
// Idf
Idf* idf_make(Allocator* alc, int type, void* item);
// Read
char* tok(Fc* fc, bool allow_space, bool allow_newline, bool update);
void tok_back(Fc* fc);
void tok_expect(Fc* fc, char* expect, bool allow_space, bool allow_newline);
char* chunk_tok(Chunk* chunk, bool allow_space, bool allow_newline, bool read_only);
char* chunk_read(Chunk* chunk, int *i_ref);
// Func
Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name);
// Class
// Value
// Token

struct Scope {
    Scope* parent;
    Map* identifiers;
};
struct Idf {
    int type;
    void* item;
};

struct Func {
    Fc* fc;
    char* name;
    char* export_name;
    Scope* scope;
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
};

#endif
