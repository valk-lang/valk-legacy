
#include "../all.h"

char* ir_value(IR* ir, Scope* scope, Value* v) {

    str_preserve(ir->block->code, 512);

    if (v->type == v_string) {
        return ir_string(ir, v->item, true);
    }
    if (v->type == v_number) {
        VNumber* item = v->item;
        if(v->rett->type == type_int || v->rett->type == type_bool)
            return ir_int(ir, item->value_int);
        // TODO: float
    }
    if (v->type == v_null) {
        return "null";
    }
    if (v->type == v_func_call) {
        VFuncCall *fcall = v->item;
        char *on = ir_value(ir, scope, fcall->on);
        Array *values = ir_fcall_args(ir, scope, fcall->args);
        char *res = ir_func_call(ir, on, values, ir_type(ir, v->rett), fcall->line, fcall->col);
        return ir_func_err_handler(ir, scope, res, fcall);
    }
    if (v->type == v_fcall_buffer) {
        VFuncCallBuffer* fbuff = v->item;
        Value *fcallv = fbuff->fcall;
        VFuncCall* fcall = fcallv->item;
        str_add(ir->block->code, "  ; FCALL BUFFER BEFORE\n");
        ir_write_ast(ir, fbuff->before);
        str_add(ir->block->code, "  ; FCALL BUFFER ON\n");
        char *on = ir_value(ir, scope, fcall->on);
        Array *values = ir_fcall_args(ir, scope, fcall->args);
        char *res = ir_func_call(ir, on, values, ir_type(ir, v->rett), fcall->line, fcall->col);
        str_add(ir->block->code, "  ; FCALL BUFFER CLEAR\n");
        ir_write_ast(ir, fbuff->after);
        return ir_func_err_handler(ir, scope, res, fcall);
    }
    if (v->type == v_func_ptr) {
        VFuncPtr *fptr = v->item;
        return ir_func_ptr(ir, fptr->func);
    }
    if (v->type == v_ptrv) {
        char *val = ir_assign_value(ir, scope, v);
        return ir_load(ir, v->rett, val);
    }
    if (v->type == v_ptr_of) {
        return ir_assign_value(ir, scope, v->item);
    }
    if (v->type == v_stack) {
        Type* type = v->rett;
        Class* class = type->class;
        // TODO: use class size
        return ir_alloca_by_size(ir, ir->func, "50");
    }
    if (v->type == v_gc_link) {
        VPair* pair = v->item;
        Value* on = pair->left;
        Value* to = pair->right;
        //
        Type *on_type = on->rett;
        char *ir_on = ir_value(ir, scope, on);
        char *ir_to = ir_value(ir, scope, to);
        char *res = ir_gc_link(ir, ir_on, ir_to);
        return res;
    }
    if (v->type == v_decl) {
        Decl *decl = v->item;
        if(decl->is_mut) {
            if (!decl->ir_store_var) {
                build_err(ir->b, "Missing decl storage variable (compiler bug)");
            }
            return ir_load(ir, decl->type, decl->ir_store_var);
        }
        if (!decl->ir_var) {
            build_err(ir->b, "Missing decl value variable (compiler bug)");
        }
        return decl->ir_var;
    }
    if (v->type == v_global) {
        Global* g = v->item;
        char* var = ir_global(ir, g);
        return ir_load(ir, g->type, var);
    }
    if (v->type == v_ir_cached) {
        VIRCached* item = v->item;
        if(item->ir_value)
            return item->ir_value;
        if(item->ir_var) {
            char* val = ir_load(ir, item->value->rett, item->ir_var);
            item->ir_value = val;
            return val;
        }
        Value* vi = item->value;
        if(value_is_assignable(vi)) {
            char* var = ir_assign_value(ir, scope, vi);
            char* val = ir_load(ir, item->value->rett, var);
            item->ir_var = var;
            item->ir_value = val;
            return val;
        }
        char* val = ir_value(ir, scope, item->value);
        item->ir_value = val;
        return val;
    }
    if (v->type == v_op) {
        VOp *vop = v->item;
        int op = vop->op;
        char *left = ir_value(ir, scope, vop->left);
        char *right = ir_value(ir, scope, vop->right);
        return ir_op(ir, scope, op, left, right, v->rett);
    }
    if (v->type == v_compare) {
        VOp *vop = v->item;
        int op = vop->op;
        char *l = ir_value(ir, scope, vop->left);
        char *r = ir_value(ir, scope, vop->right);
        Type* type = vop->left->rett;
        char *ltype = ir_type(ir, vop->left->rett);
        return ir_compare(ir, op, l, r, ltype, type->is_signed, type->type == type_float);
    }
    if (v->type == v_cast) {
        Value *val = v->item;
        Type *from_type = val->rett;
        Type *to_type = v->rett;
        char *lval = ir_value(ir, scope, val);
        return ir_cast(ir, lval, from_type, to_type);
    }
    if (v->type == v_class_init) {
        Map* values = v->item;
        Class* class = v->rett->class;
        Value* ob = NULL;
        if(class->type == ct_class) {
            ob = vgen_call_gc_alloc(ir->alc, ir->b, class->size, class->gc_fields, class);
        } else {
            ob = vgen_call_alloc(ir->alc, ir->b, class->size, class);
        }
        // TODO stack buffer variable
        char* obj = ir_value(ir, scope, ob);

        for (int i = 0; i < values->keys->length; i++) {
            char *prop_name = array_get_index(values->keys, i);
            Value *val = array_get_index(values->values, i);
            ClassProp *prop = map_get(class->props, prop_name);
            Type *type = prop->type;
            char *lval = ir_value(ir, scope, val);
            char *pvar = ir_class_pa(ir, class, obj, prop);
            ir_store_old(ir, type, pvar, lval);
        }

        return obj;
    }
    if (v->type == v_class_pa) {
        VClassPA* pa = v->item;
        Value* on = pa->on;
        char* ob = ir_value(ir, scope, on);
        char* ref = ir_class_pa(ir, on->rett->class, ob, pa->prop);
        return ir_load(ir, pa->prop->type, ref);
    }
    if (v->type == v_incr) {
        VIncr* item = v->item;
        Value* on = item->on;
        char *var = ir_assign_value(ir, scope, on);
        // char *left = ir_value(ir, scope, on);
        char *left = ir_load(ir, on->rett, var);
        char *right = ir_int(ir, 1);
        char* op = ir_op(ir, scope, item->increment ? op_add : op_sub, left, right, v->rett);
        ir_store_old(ir, v->rett, var, op);
        return item->before ? op : left;
    }
    if (v->type == v_atomic) {
        VOp *vop = v->item;

        int op = vop->op;
        char *lval1 = ir_assign_value(ir, scope, vop->left);
        char *lval2 = ir_value(ir, scope, vop->right);
        char *ltype = ir_type(ir, v->rett);
        char *var = ir_var(ir->func);

        Str *code = ir->block->code;
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, " = atomicrmw ");
        if (op == op_add) {
            str_flat(code, "add ");
        } else if (op == op_sub) {
            str_flat(code, "sub ");
        } else if (op == op_bit_and) {
            str_flat(code, "and ");
        } else if (op == op_bit_or) {
            str_flat(code, "or ");
        } else if (op == op_bit_xor) {
            str_flat(code, "xor ");
        } else {
            die("Unknown LLVM atomic operation (compiler bug)");
        }

        char bytes[10];
        str_add(code, ltype);
        str_flat(code, "* ");
        str_add(code, lval1);
        str_flat(code, ", ");
        str_add(code, ltype);
        str_flat(code, " ");
        str_add(code, lval2);
        str_flat(code, " seq_cst, align ");
        str_add(code, ir_type_align(ir, v->rett, bytes));
        str_flat(code, "\n");

        return var;
    }

    return "???";
}

char* ir_assign_value(IR* ir, Scope* scope, Value* v) {
    if (v->type == v_decl) {
        Decl *decl = v->item;
        return decl->ir_store_var;
    }
    if (v->type == v_global) {
        Global* g = v->item;
        return ir_global(ir, g);
    }
    if (v->type == v_ptrv) {
        VPtrv *ptrv = v->item;
        Value *on = ptrv->on;
        Value *index = ptrv->index;
        Type *as_type = v->rett;

        char *lval = ir_value(ir, scope, on);
        char *lindex = ir_value(ir, scope, index);
        char *lindex_type = ir_type(ir, index->rett);
        char *ltype = ir_type(ir, as_type);

        char *result = ir_var(ir->func);
        Str *code = ir->block->code;
        str_flat(code, "  ");
        str_add(code, result);
        str_flat(code, " = getelementptr inbounds ");
        str_add(code, ltype);
        str_flat(code, ", ptr ");
        str_add(code, lval);
        str_flat(code, ", ");
        str_add(code, lindex_type);
        str_flat(code, " ");
        str_add(code, lindex);
        str_flat(code, "\n");

        return result;
    }
    if (v->type == v_class_pa) {
        VClassPA* pa = v->item;
        Value* on = pa->on;
        char* ob = ir_value(ir, scope, on);
        return ir_class_pa(ir, on->rett->class, ob, pa->prop);
    }
    if (v->type == v_ir_cached) {
        VIRCached *item = v->item;
        if(item->ir_var)
            return item->ir_var;
        char *var = ir_assign_value(ir, scope, item->value);
        item->ir_var = var;
        return var;
    }
    return "?-?";
}