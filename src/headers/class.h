
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc, Build* b, int type);
int class_determine_size(Build* b, Class* class);

struct Class {
    char* name;
    char* ir_name;
    Build* b;
    Chunk* body;
    Scope* scope;
    Map* props;
    Map* funcs;
    int type;
    int size;
    bool packed;
    bool is_signed;
    bool allow_math;
};
struct ClassProp {
    Type* type;
    Chunk* chunk_type;
    Chunk* chunk_value;
    int index;
};

#endif
