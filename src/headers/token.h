
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
void token_if(Allocator* alc, Fc* fc, Scope* scope);
// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);
Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value);
Token *tgen_if(Allocator *alc, Value* cond, Scope* scope_if, Scope* scope_else);

struct Token {
    int type;
    void* item;
};
struct TDeclare {
    Decl* decl;
    Value* value;
};
struct TIf {
    Value* cond;
    Scope* scope_if;
    Scope* scope_else;
};

#endif
