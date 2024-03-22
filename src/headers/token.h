
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

Token *token_make(Allocator *alc, int type, void *item);
void token_if(Allocator* alc, Parser* p);
void token_while(Allocator* alc, Parser* p);
// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);
Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value);
Token *tgen_if(Allocator *alc, Value* cond, Scope* scope_if, Scope* scope_else);
Token *tgen_while(Allocator *alc, Value* cond, Scope* scope_while);
Token *tgen_throw(Allocator *alc, Build* b, Unit* u, FuncError* err, char* msg);
Token *tgen_each(Allocator *alc, Value *on, Func *func, Decl *kd, Decl *vd, Scope *scope, Decl* index, Value* vindex);

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
struct TEach {
    Value* on;
    Func* func;
    Scope* scope;
    Decl* kd;
    Decl* kd_buf;
    Decl* vd;
    Decl* index;
    Value* vindex;
};
struct TThrow {
    FuncError* err;
    VString* msg;
};
struct TSetRetv {
    int index;
    Value* value;
};

#endif
