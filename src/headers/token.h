
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);
Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value);

struct Token {
    int type;
    void* item;
};
struct TDeclare {
    Decl* decl;
    Value* value;
};

#endif
