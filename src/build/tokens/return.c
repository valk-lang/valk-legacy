
#include "../../all.h"

void pt_return(Build* b, Allocator* alc, Parser* p) {

    Scope* scope = p->scope;

    if (p->vscope_values) {
        Array *values = p->vscope_values;

        Type *tcv_prev = p->try_conv;
        Type *tcv = vscope_get_result_type(values);
        p->try_conv = tcv ? tcv : tcv_prev;

        Value *val = read_value(alc, p, false, 0);
        // TODO: support multiple return types

        p->try_conv = tcv_prev;

        if (tcv) {
            val = try_convert(alc, p, scope, val, tcv);
            type_check(p, tcv, val->rett);
        }
        array_push(values, val);
        array_push(scope->ast, token_make(alc, t_return_vscope, val));
        scope->did_return = true;
        return;
    }

    Func *func = p->func;
    if (!func) {
        parse_err(p, -1, "Using 'return' outside a function scope");
    }

    bool buffer_retv = func->rett_eax && type_is_gc(func->rett_eax) && func->rett_arg_types > 0;

    Type *rett = func->rett;
    Array *rett_decls = func->rett_decls;
    Array *rett_types = func->rett_types;
    int rettc = func->rett_count;
    int rett_decl_i = 0;
    Value *main_retv = NULL;
    int i = 0;
    while (i < rett_types->length) {
        if (i > 0) {
            tok_expect(p, ",", true, true);
        }
        Type *rett = array_get_index(rett_types, i);

        Type *tcv_prev = p->try_conv;
        Type *tcv = rett;
        p->try_conv = tcv;

        Value *val = read_value(alc, p, false, 0);

        p->try_conv = tcv_prev;

        val = try_convert(alc, p, p->scope, val, rett);
        type_check(p, rett, val->rett);

        if (i == 0 && func->rett_eax) {
            if (buffer_retv) {
                Decl *decl = decl_make(alc, p->func, NULL, func->rett_eax, false);
                main_retv = vgen_decl(alc, decl);
                array_push(scope->ast, tgen_declare(alc, scope, decl, val));
            } else {
                main_retv = vgen_var(alc, b, val);
                array_push(scope->ast, token_make(alc, t_set_var, main_retv->item));
            }
        } else {
            Decl *decl = array_get_index(func->rett_decls, rett_decl_i++);
            Value *var = vgen_ptrv(alc, b, vgen_decl(alc, decl), rett, vgen_int(alc, 0, NULL, type_cache_i32(b)));
            array_push(scope->ast, tgen_assign(alc, var, val));
        }
        i++;
    }

    array_push(scope->ast, tgen_return(alc, main_retv));
    scope->did_return = true;
}
