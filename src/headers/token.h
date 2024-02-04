
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);

struct Token {
    int type;
    void* item;
};

#endif
