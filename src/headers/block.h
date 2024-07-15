
#ifndef _H_BLOCK
#define _H_BLOCK

#include "typedefs.h"

Block* block_make(Allocator* alc, char* name);

struct Block {
    Array* ast;
    char* name;
};

#endif