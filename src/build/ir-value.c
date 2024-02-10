
#include "../all.h"

char* ir_value(IR* ir, Scope* scope, Value* v) {

    str_preserve(ir->block->code, 512);

    if (v->type == v_string) {
        return ir_string(ir, v->item);
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
        return res;
    }
    if (v->type == v_func_ptr) {
        VFuncPtr *fptr = v->item;
        return ir_func_ptr(ir, fptr->func);
    }
    if (v->type == v_ptrv) {
        char *val = ir_assign_value(ir, scope, v);
        return ir_load(ir, v->rett, val);
    }
    if (v->type == v_stack) {
        Type* type = v->rett;
        Class* class = type->class;
        // TODO: use class size
        return ir_alloca_by_size(ir, ir->func, "50");
    }
    if (v->type == v_decl) {
        Decl *decl = v->item;
        char *var_val = decl->ir_var;
        if (!var_val) {
            // printf("Decl name: %s\n", decl->name);
            printf("Missing decl value (compiler bug)\n");
            raise(11);
        }
        if (!decl->is_mut) {
            return var_val;
        }
        return ir_load(ir, decl->type, var_val);
    }
    if (v->type == v_global) {
        Global* g = v->item;
        char* var = ir_global(ir, g);
        return ir_load(ir, g->type, var);
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
        return ir_compare(ir, scope, op, vop->left, vop->right);
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
        Value* ob = vgen_call_alloc(ir->alc, ir->b, class->size, class);
        char* obj = ir_value(ir, scope, ob);

        for (int i = 0; i < values->keys->length; i++) {
            char *prop_name = array_get_index(values->keys, i);
            Value *val = array_get_index(values->values, i);
            ClassProp *prop = map_get(class->props, prop_name);
            Type *type = prop->type;
            char *lval = ir_value(ir, scope, val);
            char *pvar = ir_class_pa(ir, class, obj, prop);
            ir_store(ir, type, pvar, lval);
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
        ir_store(ir, v->rett, var, op);
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
        return decl->ir_var;
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
    return "?-?";
}