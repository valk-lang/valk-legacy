
#include "../all.h"

char* ir_value(IR* ir, Scope* scope, Value* v) {

    if (v->type == v_string) {
        return ir_string(ir, v->item);
    }
    if (v->type == v_int) {
        VInt* item = v->item;
        return ir_int(ir, item->value);
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
    if (v->type == v_op) {
        VOp *vop = v->item;
        int op = vop->op;
        return ir_op(ir, scope, op, vop->left, vop->right, v->rett);
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

    return "???";
}

char* ir_assign_value(IR* ir, Scope* scope, Value* v) {
    if (v->type == v_decl) {
        Decl *decl = v->item;
        return decl->ir_var;
    }
    if (v->type == v_ptrv) {
        VPtrv *ptrv = v->item;
        Value *on = ptrv->on;
        Value *index = ptrv->index;
        Type *as_type = v->rett;

        char *lval = ir_value(ir, scope, on);
        char *lindex = ir_value(ir, scope, index);
        char *lindex_type = ir_type(ir, index->rett);
        char *ltype = ir_type_real(ir, as_type);

        char *result = ir_var(ir->func);
        Str *code = ir->block->code;
        str_append_chars(code, "  ");
        str_append_chars(code, result);
        str_append_chars(code, " = getelementptr inbounds ");
        str_append_chars(code, ltype);
        str_append_chars(code, ", ptr ");
        str_append_chars(code, lval);
        str_append_chars(code, ", ");
        str_append_chars(code, lindex_type);
        str_append_chars(code, " ");
        str_append_chars(code, lindex);
        str_append_chars(code, "\n");

        return result;
    }
    // if (v->type == v_class_pa) {
    //     VClassPA *pa = v->item;
    //     Value *on = pa->on;
    //     ClassProp *prop = pa->prop;
    //     Class *class = on->rett->class;
    //     Type *type = prop->type;
    //     char *lon = llvm_value(b, scope, on);
    //     return ir_class_prop_access(b, class, lon, prop);
    // }
    return "?-?";
}