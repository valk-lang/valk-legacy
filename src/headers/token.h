
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
Token *tgen_assign(Allocator *alc, Value *left, Value *right);

struct Token {
    int type;
    void* item;
};

#endif
