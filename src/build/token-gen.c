
#include "../all.h"

void disable_token(Token* t) {
    t->type = t_disabled;
}

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

Token *tgen_throw(Allocator *alc, Build* b, Unit* u, unsigned int value, char* msg) {

    u->string_count++;
    char var[64];
    strcpy(var, "@.str.object.");
    itos(u->id, (char *)((intptr_t)var + 13), 10);
    strcat(var, "_");
    itos(u->string_count, (char *)((intptr_t)var + strlen(var)), 10);
    char* object_name = dups(b->alc, var);

    strcpy(var, "@.str.body.");
    itos(u->id, (char *)((intptr_t)var + 11), 10);
    strcat(var, "_");
    itos(u->string_count, (char *)((intptr_t)var + strlen(var)), 10);
    char* body_name = dups(b->alc, var);

    VString *str = al(b->alc, sizeof(VString));
    str->body = msg;
    str->ir_object_name = object_name;
    str->ir_body_name = body_name;

    array_push(b->strings, str);

    TThrow *item = al(alc, sizeof(TThrow));
    item->value = value;
    item->msg = str;
    return token_make(alc, t_throw, item);
}

Token *tgen_each(Allocator *alc, Parser* p, Value *on, Func *func, Decl *kd, Decl *vd, Scope *scope, Decl* index, Value* vindex) {
    TEach *item = al(alc, sizeof(TEach));
    item->on = on;
    item->func = func;
    item->scope = scope;
    item->kd = kd;
    item->kd_buf = kd;
    item->vd = vd;
    item->index = index;
    item->vindex = vindex;
    if (item->kd_buf && item->kd_buf->is_mut == false) {
        Decl *buf = decl_make(alc, p->func, NULL, kd->type, false);
        buf->is_mut = true;
        scope_add_decl(alc, scope->parent, buf);
        item->kd_buf = buf;
    }
    return token_make(alc, t_each, item);
}

Token *tgen_decl_set_store(Allocator *alc, Decl *decl, Value *val) {
    VDeclVal *pair = al(alc, sizeof(VDeclVal));
    pair->decl = decl;
    pair->value = val;
    return token_make(alc, t_decl_set_store, pair);
}
Token *tgen_decl_set_arg(Allocator *alc, Decl *decl) {
    return token_make(alc, t_decl_set_arg, decl);
}
