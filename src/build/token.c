
#include "../all.h"

Token *token_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

void token_if(Allocator* alc, Parser* p) {
    Build* b = p->b;

    Value *cond = read_value(alc, p, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        parse_err(p, -1, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
    }

    char t = tok(p, true, true, true);
    int scope_end_i = 0;
    bool single = false;
    Chunk* chunk_end = NULL;
    if (t == tok_curly_open) {
        chunk_end = chunk_clone(alc, p->chunk);
        chunk_end->i = p->scope_end_i;
    } else if (t == tok_colon) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' after the if-condition value, but found: '%s'", p->tkn);
    }

    Scope* scope = p->scope;
    Chunk* se = p->scope_end;
    Scope *scope_if = scope_sub_make(alc, sc_if, scope);
    Scope *scope_else = scope_sub_make(alc, sc_if, scope);

    scope_apply_issets(alc, scope_if, cond->issets);

    p->scope = scope_if;
    p->scope_end = chunk_end;
    read_ast(p, single);
    p->scope = scope;
    p->scope_end = se;

    t = tok(p, true, true, false);
    if(str_is(p->tkn, "else")) {
        tok(p, true, true, true);
        t = tok(p, true, true, true);
        if(str_is(p->tkn, "if")) {
            // else if
            p->scope = scope_else;
            token_if(alc, p);
            p->scope = scope;
        } else {
            // else
            bool single = false;
            chunk_end = NULL;
            if (t == tok_curly_open) {
                chunk_end = chunk_clone(alc, p->chunk);
                chunk_end->i = p->scope_end_i;
            } else if (t == tok_colon) {
                single = true;
            } else {
                parse_err(p, -1, "Expected '{' or ':' after 'else', but found: '%s'", p->tkn);
            }
            p->scope = scope_else;
            p->scope_end = chunk_end;
            read_ast(p, single);
            p->scope = scope;
            p->scope_end = se;
        }
    }

    if (!scope->ast)
        scope->ast = array_make(alc, 20);
    array_push(scope->ast, tgen_if(alc, cond, scope_if, scope_else));
}

void token_while(Allocator* alc, Parser* p) {
    // Scope* scope = p->scope;

    Value *cond = read_value(alc, p, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        parse_err(p, -1, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
    }

    char t = tok(p, true, true, true);
    bool single = false;

    Chunk* chunk_end = NULL;
    if (t == tok_curly_open) {
        chunk_end = chunk_clone(alc, p->chunk);
        chunk_end->i = p->scope_end_i;
    } else if (t == tok_colon) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' after the if-condition value, but found: '%s'", p->tkn);
    }

    Scope *scope = p->scope;
    Chunk* se = p->scope_end;
    Scope *ls = p->loop_scope;
    Scope *scope_while = scope_sub_make(alc, sc_loop, scope);

    scope_apply_issets(alc, scope_while, cond->issets);

    p->scope = scope_while;
    p->scope_end = chunk_end;
    p->loop_scope = scope_while;
    read_ast(p, single);
    p->scope = scope;
    p->scope_end = se;
    p->loop_scope = ls;

    //
    array_push(scope->ast, tgen_while(alc, cond, scope_while));
}