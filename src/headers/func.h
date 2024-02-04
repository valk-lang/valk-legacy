
#ifndef _H_FUNC
#define _H_FUNC

#include "typedefs.h"

Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name);
FuncArg* func_arg_make(Allocator* alc, Type* type);

struct Func {
    char* name;
    char* export_name;
    Build *b;
    Fc* fc;
    Scope* scope;
    //
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
    //
    Map* args;
    Type* rett;
};
struct FuncArg {
    Type* type;
    Value* value;
    Chunk* chunk_value;
};

#endif
