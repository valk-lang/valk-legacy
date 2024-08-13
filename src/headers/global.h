
#ifndef _H_GLOBAL
#define _H_GLOBAL

#include "typedefs.h"

Global* global_make(Allocator* alc, Unit* u, Fc* fc, int act, char* name, bool is_shared, bool is_const);

struct Global {
    char* name;
    char* export_name;
    Type* type;
    Value* value;
    Chunk *chunk_type;
    Chunk *chunk_value;
    Scope *declared_scope;
    Fc* fc;
    Unit* unit;
    int act;
    bool is_shared;
    bool is_used;
    bool is_const;
};

#endif