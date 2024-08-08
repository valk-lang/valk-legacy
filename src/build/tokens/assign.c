
#include "../../all.h"

void pt_assign(Build* b, Allocator* alc, Parser* p, Value* left, char t) {

    Scope* scope = p->scope;

    tok(p, true, false, true);
    if (!value_is_assignable(left)) {
        parse_err(p, -1, "Cannot assign to left side");
    }
    value_is_mutable(left);

    Type *tcv_prev = p->try_conv;
    Type *tcv = left->rett;

    p->try_conv = tcv;
    Value *right = read_value(alc, p, true, 0);
    p->try_conv = tcv_prev;

    if (type_is_void(right->rett)) {
        parse_err(p, -1, "Trying to assign a void value");
    }

    int op = op_eq;
    if (t == tok_eq) {
    } else if (t == tok_plus_eq) {
        op = op_add;
    } else if (t == tok_sub_eq) {
        op = op_sub;
    } else if (t == tok_mul_eq) {
        op = op_mul;
    } else if (t == tok_div_eq) {
        op = op_div;
    }
    if (op != op_eq) {
        right = value_handle_op(alc, p, left, right, op);
    }

    right = try_convert(alc, p, p->scope, right, left->rett);
    if (op == op_eq && left->type == v_decl_overwrite) {
        char *reason;
        if (!type_compat(left->rett, right->rett, &reason)) {
            // Remove overwrite
            DeclOverwrite *dov = left->item;
            scope_delete_idf_by_value(scope, dov, true);
            left = vgen_decl(alc, dov->decl);
        }
    }
    type_check(p, left->rett, right->rett);

    if (op == op_eq && left->type == v_global) {
        Global *g = left->item;
        if (g->is_shared && type_is_gc(g->type)) {
            // Replace right with a temporary variable
            right = vgen_var(alc, b, right);
            array_push(scope->ast, token_make(alc, t_set_var, right->item));
            // call gc_share(right)
            Func *share = get_valk_func(b, "mem", "gc_share");
            Array *args = array_make(alc, 2);
            array_push(args, right);
            Value *on = vgen_func_ptr(alc, share, NULL);
            Value *fcall = vgen_func_call(alc, p, on, args);
            array_push(scope->ast, token_make(alc, t_statement, fcall));
        }
    }

    array_push(scope->ast, tgen_assign(alc, left, right));
}
