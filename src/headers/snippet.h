
#ifndef _H_SNIP
#define _H_SNIP

#include "typedefs.h"

struct Snippet {
    Array* args;
    Chunk* chunk;
    Scope* fc_scope;
};
struct SnipArg {
    char* name;
    int type;
};

#endif
