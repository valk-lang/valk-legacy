
#ifndef H_CLASS
#define H_CLASS

#include "typedefs.h"

Class* class_make(Allocator* alc);

struct Class {
    char* name;
    Chunk* body;
    Map* props;
    Map* funcs;
    int size;
};

#endif
