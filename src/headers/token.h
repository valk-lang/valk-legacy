
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
void token_if(Allocator* alc, Fc* fc, Scope* scope);
void token_while(Allocator* alc, Fc* fc, Scope* scope);
// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);
Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value);
Token *tgen_if(Allocator *alc, Value* cond, Scope* scope_if, Scope* scope_else);
Token *tgen_while(Allocator *alc, Value* cond, Scope* scope_while);
Token *tgen_throw(Allocator *alc, Build* b, FuncError* err, char* msg);

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
struct TWhile {
    Value* cond;
    Scope* scope_while;
};
struct TThrow {
    FuncError* err;
    VString* msg;
};

#endif
