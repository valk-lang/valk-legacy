
#include "../all.h"

char* ir_value(IR* ir, Scope* scope, Value* v) {

    str_preserve(ir->block->code, 512);

    int vt = v->type;
    if (vt == v_string) {
        VString* str = v->item;
        return ir_string(ir, str);
    }
    if (vt == v_number) {
        VNumber* item = v->item;
        if(v->rett->type == type_int || v->rett->type == type_bool)
            return ir_int(ir, item->value_int);
        // TODO: float
    }
    if (vt == v_null) {
        return "null";
    }
    if (vt == v_func_call) {
        VFuncCall *fcall = v->item;
        char *on = ir_value(ir, scope, fcall->on);
        Array *values = ir_fcall_args(ir, scope, fcall->args, fcall->rett_refs);
        char *res = ir_func_call(ir, on, values, ir_type(ir, v->rett), fcall->line, fcall->col);
        return ir_func_err_handler(ir, scope, res, fcall);
    }
    if (vt == v_gc_buffer) {
        VGcBuffer* buf = v->item;
        ir_write_ast(ir, buf->scope);
        VVar* var = buf->result;
        if (!var->var) {
            build_err(ir->b, "Missing buffer v_var in IR (compiler bug)");
        }
        return var->var;
    }
    if (vt == v_func_ptr) {
        VFuncPtr *fptr = v->item;
        return ir_func_ptr(ir, fptr->func);
    }
    if (vt == v_ptrv) {
        char *val = ir_assign_value(ir, scope, v);
        return ir_load(ir, v->rett, val);
    }
    if (vt == v_ptr_offset) {
        VPtrOffset* of = v->item;
        char* on = ir_value(ir, scope, of->on);
        char* index = ir_value(ir, scope, of->index);
        char* index_type = ir_type(ir, of->index->rett);
        return ir_ptr_offset(ir, on, index, index_type, of->size);
    }
    if (vt == v_ptr_of) {
        return ir_assign_value(ir, scope, v->item);
    }
    if (vt == v_stack) {
        Type* type = v->rett;
        Class* class = type->class;
        return ir_alloca_by_size(ir, ir->func, ir_int(ir, class->size));
    }
    if (vt == v_gc_get_table) {
        Value* index = v->item;
        char* i = ir_value(ir, scope, index);
        char* mul = ir_op(ir, scope, op_mul, i, "5", index->rett);
        char* result = ir_ptrv_dyn(ir, "@volt_gc_vtable", "ptr", mul, ir_type(ir, index->rett));
        return result;
    }
    if (vt == v_gc_link) {
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
    if (vt == v_decl) {
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
    if (vt == v_global) {
        Global* g = v->item;
        char* var = ir_global(ir, g);
        return ir_load(ir, g->type, var);
    }
    if (vt == v_isset) {
        Value *on = v->item;
        char *lval = ir_value(ir, scope, on);
        return ir_notnull_i1(ir, lval);
    }
    if (vt == v_not) {
        Value *on = v->item;
        char *lval = ir_value(ir, scope, on);
        Str* code = ir->block->code;
        char* var = ir_var(ir->func);
        str_add(code, "  ");
        str_add(code, var);
        str_add(code, " = xor i1 ");
        str_add(code, lval);
        str_add(code, ", true\n");
        return var;
    }
    if (vt == v_this_or_that) {
        VThisOrThat* item = v->item;
        char* cond = ir_value(ir, scope, item->cond);
        IRBlock *block_v1 = ir_block_make(ir, ir->func, "tot_v1_");
        IRBlock *block_v2 = ir_block_make(ir, ir->func, "tot_v2_");
        IRBlock *block_after = ir_block_make(ir, ir->func, "tot_after_");
        ir_cond_jump(ir, cond, block_v1, block_v2);
        // v1
        ir->block = block_v1;
        char* v1 = ir_value(ir, scope, item->v1);
        ir_jump(ir, block_after);
        // v2
        ir->block = block_v2;
        char* v2 = ir_value(ir, scope, item->v2);
        ir_jump(ir, block_after);
        // After
        ir->block = block_after;
        char* type = ir_type(ir, v->rett);
        return ir_this_or_that(ir, v1, block_v1, v2, block_v2, type);
    }
    if (vt == v_ir_cached) {
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
    if (vt == v_ir_value) {
        return v->item;
    }
    if (vt == v_op) {
        VOp *vop = v->item;
        int op = vop->op;
        char *left = ir_value(ir, scope, vop->left);
        char *right = ir_value(ir, scope, vop->right);
        return ir_op(ir, scope, op, left, right, v->rett);
    }
    if (vt == v_compare) {
        VOp *vop = v->item;
        int op = vop->op;
        char *l = ir_value(ir, scope, vop->left);
        char *r = ir_value(ir, scope, vop->right);
        Type* type = vop->left->rett;
        char *ltype = ir_type(ir, vop->left->rett);
        return ir_compare(ir, op, l, r, ltype, type->is_signed, type->type == type_float);
    }
    if (vt == v_cast) {
        Value *val = v->item;
        Type *from_type = val->rett;
        Type *to_type = v->rett;
        char *lval = ir_value(ir, scope, val);
        return ir_cast(ir, lval, from_type, to_type);
    }
    if (vt == v_class_init) {
        Map* values = v->item;
        Class* class = v->rett->class;
        Value* ob = NULL;

        // Write prop values
        Array* ir_props = array_make(ir->alc, values->keys->length);
        for (int i = 0; i < values->keys->length; i++) {
            Value *val = array_get_index(values->values, i);
            char *lval = ir_value(ir, scope, val);
            array_push(ir_props, lval);
        }

        // Alloc memory
        if(class->type == ct_class) {
            ob = vgen_call_gc_alloc(ir->alc, ir->b, class->size, class);
        } else {
            ob = vgen_call_alloc(ir->alc, ir->b, class->size, class);
        }
        char* obj = ir_value(ir, scope, ob);

        // Set props
        for (int i = 0; i < ir_props->length; i++) {
            char* lval = array_get_index(ir_props, i);
            char *prop_name = array_get_index(values->keys, i);
            ClassProp *prop = map_get(class->props, prop_name);
            char *pvar = ir_class_pa(ir, class, obj, prop);
            ir_store_old(ir, prop->type, pvar, lval);
        }

        return obj;
    }
    if (vt == v_class_pa) {
        VClassPA* pa = v->item;
        Value* on = pa->on;
        char* ob = ir_value(ir, scope, on);
        char* ref = ir_class_pa(ir, on->rett->class, ob, pa->prop);
        return ir_load(ir, pa->prop->type, ref);
    }
    if (vt == v_incr) {
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
    if (vt == v_atomic) {
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
    if (vt == v_var) {
        VVar* vv = v->item;
        char* result = vv->var;
        if(result == NULL) {
            build_err(ir->b, "Missing value for v_var (compiler bug)");
        }
        return result;
    }
    if (vt == v_and_or) {
        VOp* vop = v->item;

        char* left = ir_value(ir, scope, vop->left);
        IRBlock *block_current = ir->block;

        IRBlock *block_right = ir_block_make(ir, ir->func, "and_or_next_");
        ir->block = block_right;
        char* right = ir_value(ir, scope, vop->right);
        IRBlock *block_last = ir->block;

        ir->block = block_current;
        return ir_and_or(ir, block_current, left, block_right, right, block_last, vop->op);
    }

    return "???";
}

char* ir_assign_value(IR* ir, Scope* scope, Value* v) {
    int vt = v->type;
    if (vt == v_decl) {
        Decl *decl = v->item;
        return decl->ir_store_var;
    }
    if (vt == v_global) {
        Global* g = v->item;
        return ir_global(ir, g);
    }
    if (vt == v_ptrv) {
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
    if (vt == v_class_pa) {
        VClassPA* pa = v->item;
        Value* on = pa->on;
        char* ob = ir_value(ir, scope, on);
        return ir_class_pa(ir, on->rett->class, ob, pa->prop);
    }
    if (vt == v_ir_cached) {
        VIRCached *item = v->item;
        if(item->ir_var)
            return item->ir_var;
        char *var = ir_assign_value(ir, scope, item->value);
        item->ir_var = var;
        return var;
    }
    return "?-?";
}