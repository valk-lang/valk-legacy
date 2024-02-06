
#include "../all.h"

Token *token_make(Allocator *alc, int type, void *item) {
    Token *t = al(alc, sizeof(Token));
    t->type = type;
    t->item = item;
    return t;
}

void token_if(Allocator* alc, Fc* fc, Scope* scope) {
    Build* b = fc->b;

    Value *cond = read_value(alc, fc, scope, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        sprintf(b->char_buf, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    char* tkn = tok(fc, true, true, true);
    bool single = false;
    if (str_is(tkn, "{")) {
    } else if (str_is(tkn, ":")) {
        single = true;
    } else {
        sprintf(b->char_buf, "Expected '{' or ':' after the if-condition value, but found: '%s'", tkn);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Scope *scope_if = scope_sub_make(alc, sc_default, scope);
    Scope *scope_else = scope_sub_make(alc, sc_default, scope);

    read_ast(fc, scope_if, single);

    tkn = tok(fc, true, true, true);
    if(str_is(tkn, "else")) {
        tkn = tok(fc, true, true, true);
        if(str_is(tkn, "if")) {
            // else if
            token_if(alc, fc, scope_else);
        } else {
            // else
            bool single = false;
            if (str_is(tkn, "{")) {
            } else if (str_is(tkn, ":")) {
                single = true;
            } else {
                sprintf(b->char_buf, "Expected '{' or ':' after 'else', but found: '%s'", tkn);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            read_ast(fc, scope_else, single);
        }
    } else {
        tok_back(fc);
    }

    array_push(scope->ast, tgen_if(alc, cond, scope_if, scope_else));
}

void token_while(Allocator* alc, Fc* fc, Scope* scope) {
    Build* b = fc->b;

    Value *cond = read_value(alc, fc, scope, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        sprintf(b->char_buf, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    char* tkn = tok(fc, true, true, true);
    bool single = false;

    if (str_is(tkn, "{")) {
    } else if (str_is(tkn, ":")) {
        single = true;
    } else {
        sprintf(b->char_buf, "Expected '{' or ':' after the if-condition value, but found: '%s'", tkn);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Scope *scope_while = scope_sub_make(alc, sc_default, scope);

    read_ast(fc, scope_while, single);

    array_push(scope->ast, tgen_while(alc, cond, scope_while));
}