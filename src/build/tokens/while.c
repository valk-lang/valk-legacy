
#include "../../all.h"

void pt_while(Build* b, Allocator* alc, Parser* p) {

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

    Scope *scope = p->scope;
    Scope *ls = p->loop_scope;
    Scope *scope_while = scope_sub_make(alc, sc_loop, scope);

    scope_apply_issets(alc, scope_while, cond->issets);

    p->scope = scope_while;
    p->loop_scope = scope_while;
    read_ast(p, single);
    p->scope = scope;
    p->loop_scope = ls;
    if (!single)
        p->chunk->i = scope_end_i;

    //
    array_push(scope->ast, tgen_while(alc, cond, scope_while));
}