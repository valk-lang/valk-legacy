
#ifndef _H_SNIP
#define _H_SNIP

#include "typedefs.h"

Scope* gen_snippet_ast(Allocator* alc, Fc* fc, Snippet* snip, Map* idfs, Scope* scope_parent);
void read_snippet_ast(Allocator* alc, Fc* fc, Scope* scope, Snippet* snip);

struct Snippet {
    Array* args;
    Chunk* chunk;
    Scope* fc_scope;
    Array* exports;
};
struct SnipArg {
    char* name;
    int type;
};

#endif
