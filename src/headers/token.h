
#ifndef H_TOKEN
#define H_TOKEN

#include "typedefs.h"

void disable_token(Token* t);
Token *token_make(Allocator *alc, int type, void *item);

void pt_let(Build *b, Allocator *alc, Parser *p);
void pt_if(Build* b, Allocator* alc, Parser* p);
void pt_while(Build* b, Allocator* alc, Parser* p);
void pt_return(Build* b, Allocator* alc, Parser* p);
void pt_throw(Build* b, Allocator* alc, Parser* p);
void pt_each(Build* b, Allocator* alc, Parser* p);
void pt_await_fd(Build* b, Allocator* alc, Parser* p);
void pt_await_last(Build* b, Allocator* alc, Parser* p);
void pt_gc_share(Build* b, Allocator* alc, Parser* p);
void pt_assign(Build* b, Allocator* alc, Parser* p, Value* left, char t);

// Generate
Token *tgen_assign(Allocator *alc, Value *left, Value *right);
Token *tgen_return(Allocator *alc, Value *value);
Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value);
Token *tgen_if(Allocator *alc, Value* cond, Scope* scope_if, Scope* scope_else);
Token *tgen_while(Allocator *alc, Value* cond, Scope* scope_while);
Token *tgen_throw(Allocator *alc, Build* b, Unit* u, unsigned int value, char* msg);
Token *tgen_each(Allocator *alc, Parser* p, Value *on, Func *func, Decl *kd, Decl *vd, Scope *scope, Decl* index, Value* vindex);

Token *tgen_decl_set_store(Allocator *alc, Decl *decl, Value *val);
Token *tgen_decl_set_arg(Allocator *alc, Decl *decl);

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
    VString* msg;
    unsigned int value;
};
struct TSetRetv {
    int index;
    Value* value;
};

#endif
