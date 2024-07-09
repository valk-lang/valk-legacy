
#include "../../all.h"

void pt_let(Build *b, Allocator *alc, Parser *p) {

    Scope* scope = p->scope;
    Array *names = array_make(alc, 2);
    Array *types = array_make(alc, 2);
    char t;

    while (true) {
        t = tok(p, true, false, true);
        char *name = p->tkn;
        if (t != tok_id) {
            parse_err(p, -1, "Invalid variable name: '%s'", name);
        }
        t = tok(p, true, false, true);
        Type *type = NULL;
        if (t == tok_colon) {
            type = read_type(p, alc, true);
            t = tok(p, true, false, true);
            if (type_is_void(type)) {
                parse_err(p, -1, "Variables cannot have a 'void' type, variable: '%s'", name);
            }
        }
        array_push(names, name);
        array_push(types, type);

        if (t != tok_comma)
            break;
    }
    if (t != tok_eq) {
        parse_err(p, -1, "Expected '=' here, found: '%s'", p->tkn);
    }

    Type *tcv_prev = p->try_conv;
    Type *tcv = array_get_index(types, 0);
    p->try_conv = tcv;

    Value *val = read_value(alc, p, true, 0);

    p->try_conv = tcv_prev;

    if (type_is_void(val->rett)) {
        parse_err(p, -1, "Right side returns a 'void' value");
    }

    Array *values = all_values(alc, val);

    loop(names, i) {
        if (i == values->length) {
            parse_err(p, -1, "Right side does not return enough values to fit all your variables");
        }

        char *name = array_get_index(names, i);
        Type *type = array_get_index(types, i);

        Value *right = array_get_index(values, i);
        if (type) {
            right = try_convert(alc, p, scope, right, type);
            type_check(p, type, right->rett);
        } else {
            type = right->rett;
        }

        Decl *decl = decl_make(alc, p->func, name, type, false);
        Idf *idf = idf_make(b->alc, idf_decl, decl);
        scope_set_idf(scope, name, idf, p);

        array_push(scope->ast, tgen_declare(alc, scope, decl, right));
    }
}
