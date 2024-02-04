
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc, Build* b);

struct Class {
    char* name;
    Build* b;
    Chunk* body;
    Map* props;
    Map* funcs;
    int size;
};
struct ClassProp {
    Type* type;
};

#endif
