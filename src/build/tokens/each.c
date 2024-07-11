
#include "../../all.h"

void pt_each(Build* b, Allocator* alc, Parser* p) {

    char t;

    Value *on = read_value(alc, p, true, 0);
    Func *func = on->rett->class ? map_get(on->rett->class->funcs, "_each") : NULL;
    if (!func) {
        parse_err(p, -1, "Cannot use 'each' on this value. The class of it's type does not contain an '_each' method");
    }
    func_mark_used(p->func, func);

    tok_expect(p, "as", true, true);
    t = tok(p, true, false, true);
    if (t != tok_id) {
        parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
    }
    char *kname = NULL;
    char *vname = p->tkn;
    t = tok(p, true, false, false);
    if (t == tok_comma) {
        tok(p, true, false, true);
        kname = vname;
        t = tok(p, true, false, true);
        if (t != tok_id) {
            parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
        }
        vname = p->tkn;
    }
    if (kname && str_is(kname, vname)) {
        parse_err(p, -1, "Key/value variable names cannot be the same: '%s'", kname);
    }

    t = tok(p, true, true, true);
    int scope_end_i = -1;
    bool single = false;
    if (t == tok_curly_open) {
        scope_end_i = p->scope_end_i;
    } else if (t == tok_colon) {
        single = true;
    } else {
        parse_err(p, -1, "Expected '{' or ':' at the end of the 'each' statement, but found: '%s'", p->tkn);
    }

    Scope *scope = p->scope;
    Scope *ls = p->loop_scope;
    Scope *scope_each = scope_sub_make(alc, sc_loop, scope);

    Decl *on_decl = decl_make(alc, p->func, NULL, on->rett, false);
    on_decl->is_mut = true;
    array_push(scope->ast, tgen_declare(alc, scope, on_decl, on));

    Decl *kd = decl_make(alc, p->func, kname, array_get_index(func->rett_types, 1), false);
    kd->is_mut = true;
    Idf *idf = idf_make(b->alc, idf_decl, kd);
    scope_add_decl(alc, scope, kd);
    if (kname) {
        scope_set_idf(scope_each, kname, idf, p);
    }

    Decl *vd = decl_make(alc, p->func, vname, array_get_index(func->rett_types, 0), false);
    vd->is_mut = true;
    idf = idf_make(b->alc, idf_decl, vd);
    scope_set_idf(scope_each, vname, idf, p);
    scope_add_decl(alc, scope, vd);
    // Index
    Decl *index = decl_make(alc, p->func, NULL, type_gen_valk(alc, b, "uint"), false);
    index->is_mut = true;
    scope_add_decl(alc, scope, index);
    Value *vindex = value_make(alc, v_decl, index, index->type);
    // Set index to 0
    array_push(scope->ast, tgen_assign(alc, vindex, vgen_int(alc, 0, index->type)));

    p->scope = scope_each;
    p->loop_scope = scope_each;
    read_ast(p, single);
    p->scope = scope;
    p->loop_scope = ls;
    if (!single)
        p->chunk->i = scope_end_i;

    array_push(scope->ast, tgen_each(alc, p, vgen_decl(alc, on_decl), func, kd, vd, scope_each, index, vindex));
}