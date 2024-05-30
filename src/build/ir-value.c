
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
        return ir_float(ir, item->value_float);
    }
    if (vt == v_null) {
        return "null";
    }
    if (vt == v_func_call) {
        VFuncCall *fcall = v->item;
        char *on = ir_value(ir, scope, fcall->on);
        Array *values = ir_fcall_args(ir, scope, fcall->args, fcall->rett_refs);
        char *call = ir_func_call(ir, on, values, ir_type(ir, v->rett), fcall->line, fcall->col);

        ErrorHandler* errh = fcall->errh;
        if (errh) {
            return ir_func_err_handler(ir, scope, errh, call, false);
        }
        return call;
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
        Value* val = v->item;
        char* size = ir_value(ir, scope, val);
        char* type = ir_type(ir, val->rett);
        return ir_alloca_by_size(ir, ir->func, type, size);
    }
    if (vt == v_gc_get_table) {
        Value* index = v->item;
        char* i = ir_value(ir, scope, index);
        char* mul = ir_op(ir, scope, op_mul, i, "5", index->rett);
        char* result = ir_ptrv_dyn(ir, "@valk_gc_vtable", "ptr", mul, ir_type(ir, index->rett));
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
        char *res = ir_gc_link(ir, ir_on, ir_to, to->rett->nullable);
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
    if (vt == v_decl_overwrite) {
        DeclOverwrite *dov = v->item;
        Decl *decl = dov->decl;
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
        block_v1 = ir->block;
        ir_jump(ir, block_after);
        // v2
        ir->block = block_v2;
        char* v2 = ir_value(ir, scope, item->v2);
        block_v2 = ir->block;
        ir_jump(ir, block_after);
        // After
        ir->block = block_after;
        char* type = ir_type(ir, v->rett);
        return ir_this_or_that(ir, v1, block_v1, v2, block_v2, type);
    }
    if (vt == v_null_alt_value) {
        VPair* item = v->item;
        char* left = ir_value(ir, scope, item->left);
        char* cmp = ir_compare(ir, op_eq, left, "null", "ptr", false, false);
        IRBlock *current = ir->block;
        IRBlock *block_alt = ir_block_make(ir, ir->func, "null_alt_");
        IRBlock *block_after = ir_block_make(ir, ir->func, "null_after_");
        ir_cond_jump(ir, cmp, block_alt, block_after);

        ir->block = block_alt;
        char* alt = ir_value(ir, scope, item->right);
        block_alt = ir->block;
        ir_jump(ir, block_after);

        ir->block = block_after;
        char* type = ir_type(ir, v->rett);
        return ir_this_or_that(ir, left, current, alt, block_alt, type);

        Value* right = item->right;
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
    if (vt == v_cached_stack_adr) {
        return ir->func->var_g_stack_adr;
    }
    if (vt == v_cached_stack_instance) {
        return ir->func->var_g_stack;
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
        VClassInit* ci = v->item;
        Value* ob = ci->item;
        Map* values = ci->prop_values;
        Class* class = v->rett->class;

        // Write prop values
        Array* ir_props = array_make(ir->alc, values->keys->length);
        for (int i = 0; i < values->keys->length; i++) {
            Value *val = array_get_index(values->values, i);
            char *lval = ir_value(ir, scope, val);
            array_push(ir_props, lval);
        }

        // Init object
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
    if (vt == v_vscope) {
        Scope* vscope = v->item;
        IRBlock* _prev_after = ir->vscope_after;
        Array* _prev_values = ir->vscope_values;
        Array* values = array_make(ir->alc, 2);
        IRBlock *block_after = ir_block_make(ir, ir->func, "vscope_after_");
        ir->vscope_values = values;
        ir->vscope_after = block_after;
        ir_write_ast(ir, vscope);
        ir->vscope_values = _prev_values;
        ir->vscope_after = _prev_after;
        // After
        ir->block = block_after;
        return ir_phi(ir, values, ir_type(ir, v->rett));
    }
    // if (vt == v_await) {
    //     VAwait* aw = v->item;
    //     return ir_await(ir, scope, aw);
    // }
    if (vt == v_frameptr) {
        Str *code = ir->block->code;
        char* framep = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, framep);
        str_flat(code, " = tail call ptr @llvm.frameaddress(i32 0)\n");
        return framep;
    }
    if (vt == v_stackptr) {
        Str *code = ir->block->code;
        char* ptr = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, ptr);
        str_flat(code, " = tail call ptr @llvm.stacksave()\n");
        return ptr;
    }
    if (vt == v_setjmp) {
        Value* val = v->item;
        char* buf = ir_value(ir, scope, val);
        char* framep = ir_var(ir->func);
        char* stackp = ir_var(ir->func);
        char* result = ir_var(ir->func);
        Str *code = ir->block->code;
        // Frame pointer
        str_flat(code, "  ");
        str_add(code, framep);
        str_flat(code, " = tail call ptr @llvm.frameaddress(i32 0)\n");
        char* s1 = ir_ptrv(ir, buf, "ptr", 0);
        ir_store(ir, s1, framep, "ptr", ir->b->ptr_size * 2);

        // // Stack pointer
        str_flat(code, "  ");
        str_add(code, stackp);
        str_flat(code, " = tail call ptr @llvm.stacksave()\n");
        char* s2 = ir_ptrv(ir, buf, "ptr", 2);
        ir_store(ir, s2, stackp, "ptr", ir->b->ptr_size * 2);

        // Call setjmp
        str_flat(code, "  ");
        str_add(code, result);
        str_flat(code, " = tail call i32 @llvm.eh.sjlj.setjmp(ptr ");
        str_add(code, buf);
        str_flat(code, ")\n");
        return result;
    }
    if (vt == v_longjmp) {
        Value* val = v->item;
        char* buf = ir_value(ir, scope, val);
        Str *code = ir->block->code;
        str_flat(code, "  call void @llvm.eh.sjlj.longjmp(ptr ");
        str_add(code, buf);
        str_flat(code, ")\n");
        str_flat(code, "unreachable\n");
        return "";
    }

    printf("unhandled ir-value: '%d' (compiler bug)\n", vt);
    exit(1);
}

char* ir_assign_value(IR* ir, Scope* scope, Value* v) {
    int vt = v->type;
    if (vt == v_decl) {
        Decl *decl = v->item;
        return decl->ir_store_var;
    }
    if (vt == v_decl_overwrite) {
        DeclOverwrite *dov = v->item;
        Decl *decl = dov->decl;
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
    if (vt == v_cached_stack_adr) {
        return ir->func->var_g_stack_adr_ref;
    }
    return "?-?";
}