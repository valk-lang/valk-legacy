
#include "../../all.h"

void pt_if(Build* b, Allocator* alc, Parser* p) {

    Value *cond = read_value(alc, p, false, 0);
    if (!type_is_bool(cond->rett)) {
        char buf[256];
        parse_err(p, -1, "if condition value must return a bool type, but found: '%s'", type_to_str(cond->rett, buf));
    }

    char t = tok(p, true, true, true);
    int scope_end_i = -1;
    bool single = false;
    if (t == tok_curly_open) {
        scope_end_i = p->scope_end_i;
    } else if (t == tok_colon) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' after the if-condition value, but found: '%s'", p->tkn);
    }

    Scope* scope = p->scope;
    Scope *scope_if = scope_sub_make(alc, sc_if, scope);
    Scope *scope_else = scope_sub_make(alc, sc_if, scope);

    scope_apply_issets(alc, scope_if, cond->issets);

    p->scope = scope_if;
    read_ast(p, single);
    p->scope = scope;
    if(!single)
        p->chunk->i = scope_end_i;

    if(scope_if->did_return) {
        scope_apply_issets(alc, scope, cond->not_issets);
    }
    scope_apply_issets(alc, scope_else, cond->not_issets);

    t = tok(p, true, true, false);
    if(str_is(p->tkn, "else")) {
        tok(p, true, true, true);
        t = tok(p, true, true, true);
        if(str_is(p->tkn, "if")) {
            // else if
            p->scope = scope_else;
            pt_if(b, alc, p);
            p->scope = scope;
        } else {
            // else
            int scope_end_i = -1;
            bool single = false;
            if (t == tok_curly_open) {
                scope_end_i = p->scope_end_i;
            } else if (t == tok_colon) {
                single = true;
            } else {
                parse_err(p, -1, "Expected '{' or ':' after 'else', but found: '%s'", p->tkn);
            }
            p->scope = scope_else;
            read_ast(p, single);
            p->scope = scope;
            if(!single)
                p->chunk->i = scope_end_i;
        }
    }

    if (!scope->ast)
        scope->ast = array_make(alc, 20);
    array_push(scope->ast, tgen_if(alc, cond, scope_if, scope_else));
}
