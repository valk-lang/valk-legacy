
#include "../../all.h"

void pt_throw(Build *b, Allocator *alc, Parser *p) {

    Scope* scope = p->scope;

    char t = tok(p, true, false, true);
    if (t != tok_id) {
        parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
    }
    char *name = p->tkn;
    Func *func = p->func;
    Scope *fscope = func->scope;
    unsigned int err = 0;
    if (func->errors) {
        err = (unsigned int)(intptr_t)map_get(func->errors, name);
    }
    if (err == 0) {
        parse_err(p, -1, "Function has no error defined named: '%s'", name);
    }

    array_push(scope->ast, tgen_throw(alc, b, p->unit, err, name));
    scope->did_return = true;
}