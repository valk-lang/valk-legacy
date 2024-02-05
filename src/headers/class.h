
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc, Build* b, int type);

struct Class {
    char* name;
    char* ir_name;
    Build* b;
    Chunk* body;
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
};

#endif
