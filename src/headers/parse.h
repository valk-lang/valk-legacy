
#ifndef _H_PARSE
#define _H_PARSE

#include "typedefs.h"

Scope* scope_make(Allocator* alc, Scope* parent);
Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name);

struct Scope {
    Scope* parent;
    Map* identifiers;
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
