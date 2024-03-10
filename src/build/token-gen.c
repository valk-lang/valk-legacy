
#include "../all.h"

Token *tgen_assign(Allocator *alc, Value *left, Value *right) {
    VPair *pair = al(alc, sizeof(VPair));
    pair->left = left;
    pair->right = right;
    return token_make(alc, t_assign, pair);
}

Token *tgen_return(Allocator *alc, Value *value) {
    return token_make(alc, t_return, value);
}

Token *tgen_declare(Allocator *alc, Scope* scope, Decl *decl, Value* value) {
    scope_add_decl(alc, scope, decl);
    TDeclare *item = al(alc, sizeof(TDeclare));
    item->decl = decl;
    item->value = value;
    return token_make(alc, t_declare, item);
}

Token *tgen_if(Allocator *alc, Value* cond, Scope* scope_if, Scope* scope_else) {
    TIf *item = al(alc, sizeof(TIf));
    item->cond = cond;
    item->scope_if = scope_if;
    item->scope_else = scope_else;
    return token_make(alc, t_if, item);
}

Token *tgen_while(Allocator *alc, Value* cond, Scope* scope_while) {
    TWhile *item = al(alc, sizeof(TWhile));
    item->cond = cond;
    item->scope_while = scope_while;
    return token_make(alc, t_while, item);
}

Token *tgen_throw(Allocator *alc, Build* b, FuncError* err, char* msg) {

    b->string_count++;
    char var[64];
    strcpy(var, "@.str.object.");
    itoa(b->string_count, (char *)((intptr_t)var + 13), 10);
    char* object_name = dups(b->alc, var);

    strcpy(var, "@.str.body.");
    itoa(b->string_count, (char *)((intptr_t)var + 11), 10);
    char* body_name = dups(b->alc, var);

    VString *str = al(b->alc, sizeof(VString));
    str->body = msg;
    str->ir_object_name = object_name;
    str->ir_body_name = body_name;

    array_push(b->strings, str);

    TThrow *item = al(alc, sizeof(TThrow));
    item->err = err;
    item->msg = str;
    return token_make(alc, t_throw, item);
}