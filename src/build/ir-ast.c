
#include "../all.h"

void ir_write_ast(IR* ir, Scope* scope) {

    Allocator *alc = ir->alc;
    str_preserve(ir->block->code, 512);

    if(!scope->ast)
        return;

    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);
        int tt = t->type;

        if (tt == t_disabled) {
            continue;
        }
        if (tt == t_statement) {
            Value *v = t->item;
            ir_value(ir, v);
            continue;
        }
        if (tt == t_decl_set_store) {
            VDeclVal *item = t->item;
            char* store = ir_value(ir, item->value);
            item->decl->ir_store = store;
            continue;
        }
        if (tt == t_decl_set_arg) {
            Decl *decl = t->item;
            if(decl->ir_store) {
                char* arg = ir_arg_nr(ir, decl->arg_nr);
                ir_store(ir, decl->ir_store, arg, ir_type(ir, decl->type), decl->type->size);
            }
            if(!decl->is_mut) {
                decl->ir_var = ir_arg_nr(ir, decl->arg_nr);
            }
            continue;
        }
        if (tt == t_declare) {
            TDeclare* item = t->item;
            Decl* decl = item->decl;
            char* val = ir_value(ir, item->value);
            ir_decl_store(ir, decl, val);
            continue;
        }
        if (tt == t_assign) {
            VPair* pair = t->item;
            Value* left = pair->left;
            Value* right = pair->right;
            char* value = ir_value(ir, right);
            if (left->type == v_class_pa && type_is_gc(left->rett) && type_is_gc(right->rett)) {
                // GC link
                VClassPA *pa = left->item;
                Type* on_type = pa->on->rett;
                char* on = ir_value(ir, pa->on);
                char* var = ir_class_pa(ir, on_type->class, on, pa->prop);
                char* result = ir_gc_link(ir, on, value, right->rett->nullable);
                ir_store(ir, var, result, "ptr", ir->b->ptr_size);
            } else {
                char* var = ir_assign_value(ir, left);
                ir_store_old(ir, left->rett, var, value);
            }
            continue;
        }

        if (tt == t_return) {
            Value *v = t->item;
            if(v) {
                char* irv = ir_value(ir, v);
                ir_func_return(ir, scope, ir_type(ir, v->rett), irv);
            } else {
                ir_func_return_nothing(ir, scope);
            }
            continue;
        }

        if (tt == t_if) {
            TIf *ift = t->item;
            ir_if(ir, ift);
            continue;
        }

        if (tt == t_while) {
            TWhile *item = t->item;
            ir_while(ir, item);
            continue;
        }
        if (tt == t_break) {
            Scope* loop_scope = t->item;
            if (loop_scope->defer) {
                ir_write_ast(ir, loop_scope->defer);
            }
            IRBlock* after = ir->block_after;
            if(!after) {
                die("Missing IR after block for 'break' (compiler bug)");
            }
            ir_jump(ir, after);
            continue;
        }
        if (tt == t_continue) {
            Scope* loop_scope = t->item;
            if (loop_scope->defer) {
                ir_write_ast(ir, loop_scope->defer);
            }
            IRBlock* cond = ir->block_cond;
            if(!cond) {
                die("Missing IR condition block for 'break' (compiler bug)");
            }
            ir_jump(ir, cond);
            continue;
        }
        if (tt == t_throw) {
            TThrow* tt = t->item;
            ir_store_old(ir, type_gen_valk(alc, ir->b, "i32"), "@valk_err_code", ir_int(ir, tt->value));
            char *msg = ir_string(ir, tt->msg);
            ir_store_old(ir, type_gen_valk(alc, ir->b, "ptr"), "@valk_err_msg", msg);
            ir_func_return_nothing(ir, scope);
            continue;
        }
        if (tt == t_return_vscope) {
            char* val = ir_value(ir, t->item);
            if(!ir->vscope_values) {
                die("Missing IR value-scope values array (compiler bug)");
            }
            IRPhiValue* v = al(ir->alc, sizeof(IRPhiValue));
            v->value = val;
            v->block = ir->block;
            array_push(ir->vscope_values, v);
            ir_jump(ir, ir->vscope_after);
            continue;
        }
        if (tt == t_set_var) {
            VVar *vv = t->item;
            vv->var = ir_value(ir, vv->value);
            continue;
        }
        if (tt == t_ast_scope) {
            Scope *s = t->item;
            ir_write_ast(ir, s);
            continue;
        }
        // if (tt == t_set_return_value) {
        //     die("TODO: DELETE\n");
        //     TSetRetv* sr = t->item;
        //     int index = sr->index;
        //     Value* val = sr->value;
        //     Array* rett_refs = ir->func->rett_refs;
        //     char* var = array_get_index(rett_refs, index);
        //     if(!var) {
        //         build_err(ir->b, "Missing return value IR variable (compiler bug)");
        //     }
        //     char* type = ir_type(ir, val->rett);
        //     IRBlock *block_if = ir_block_make(ir, ir->func, "if_set_ret_");
        //     IRBlock *after = ir_block_make(ir, ir->func, "set_ret_after_");
        //     char* nn = ir_notnull_i1(ir, var);
        //     ir_cond_jump(ir, nn, block_if, after);
        //     ir->block = block_if;
        //     ir_store(ir, var, ir_value(ir, val), type, val->rett->size);
        //     ir_jump(ir, after);
        //     ir->block = after;
        //     continue;
        // }
        if (tt == t_each) {
            TEach* item = t->item;
            IRBlock *block_cond = ir_block_make(ir, ir->func, "each_cond_");
            IRBlock *block_code = ir_block_make(ir, ir->func, "each_code_");
            IRBlock *block_after = ir_block_make(ir, ir->func, "each_after_");
            ir_jump(ir, block_cond);
            ir->block = block_cond;

            char* on = ir_value(ir, item->on);

            Decl* kd = item->kd;
            Decl* kd_buf = item->kd_buf;
            Decl* vd = item->vd;
            Decl* index = item->index;

            Type* type_ptr = type_gen_valk(ir->alc, ir->b, "ptr");
            Array *types = array_make(ir->alc, 4);
            array_push(types, item->on->rett);
            array_push(types, index->type);
            array_push(types, type_ptr);
            Array *values = array_make(ir->alc, 4);
            array_push(values, on);
            array_push(values, ir_value(ir, vgen_decl(alc, index)));
            array_push(values, ir_value(ir, kd_buf ? value_make(alc, v_ptr_of, value_make(alc, v_decl, kd_buf, kd_buf->type), type_ptr) : vgen_null(alc, ir->b)));
            Array *args = ir_fcall_ir_args(ir, values, types);
            //
            char* fptr = ir_func_ptr(ir, item->func);
            char* fcall = ir_func_call(ir, fptr, args, ir_type(ir, item->func->rett_eax), 0, 0);

            if(kd && !kd->is_mut) {
                ir_decl_store(ir, kd, ir_value(ir, vgen_decl(alc, kd_buf)));
            }
            ir_decl_store(ir, vd, fcall);

            // Increment index
            char* incr = ir_op(ir, op_add, ir_value(ir, vgen_decl(alc, index)), ir_int(ir, 1), index->type);
            ir_store(ir, ir_assign_value(ir, vgen_decl(alc, index)), incr, ir_type(ir, index->type), index->type->size);
            // Cond
            Type *type_i32 = type_gen_valk(ir->alc, ir->b, "i32");
            char *load = ir_load(ir, type_i32, "@valk_err_code");
            char *lcond = ir_compare(ir, op_eq, load, "0", "i32", false, false);
            // Clear error
            ir_store_old(ir, type_i32, "@valk_err_code", "0");
            ir_cond_jump(ir, lcond, block_code, block_after);
            //
            ir->block = block_code;
            IRBlock *backup_after = ir->block_after;
            IRBlock *backup_cond = ir->block_cond;
            ir->block_after = block_after;
            ir->block_cond = block_cond;
            ir_write_ast(ir, item->scope);
            ir->block_after = backup_after;
            ir->block_cond = backup_cond;
            if(!item->scope->did_return) {
                ir_jump(ir, block_cond);
            }

            ir->block = block_after;
            // Clear from stack
            if (kd && kd->is_gc && kd->is_mut) {
                ir_decl_store(ir, kd, "null");
            }
            if (kd_buf && kd_buf->is_gc && kd_buf->is_mut) {
                ir_decl_store(ir, kd_buf, "null");
            }
            if (vd->is_gc && vd->is_mut) {
                ir_decl_store(ir, vd, "null");
            }
            //
            continue;
        }

        die("Unhandled IR token (compiler bug)");
    }

    // Defer
    if(scope->defer && !scope->did_return) {
        ir_write_ast(ir, scope->defer);
    }
}

char* ir_gc_link(IR* ir, char* on, char* to, bool nullable) {
    Build* b = ir->b;

    Type* type_u8 = type_gen_valk(ir->alc, b, "u8");
    Type* type_ptr = type_gen_valk(ir->alc, b, "ptr");

    IRBlock *current = ir->block;
    IRBlock *block_not_null = nullable ? ir_block_make(ir, ir->func, "if_link_not_null_") : NULL;
    IRBlock *block_if = ir_block_make(ir, ir->func, "if_link_");
    IRBlock *after = ir_block_make(ir, ir->func, "link_after_");

    // Not null
    if(block_not_null) {
        char *comp = ir_compare(ir, op_ne, to, "null", "ptr", false, false);
        ir_cond_jump(ir, comp, block_not_null, after);
        ir->block = block_not_null;
    }

    // On state > transfer
    char* on_state_var = ir_ptrv(ir, on, "i8", -8);
    char* on_state = ir_load(ir, type_u8, on_state_var);
    char *comp_on = ir_compare(ir, op_gt, on_state, "2", "i8", false, false);
    ir_cond_jump(ir, comp_on, block_if, after);

    ir->block = block_if;
    Func *func = get_valk_class_func(b, "mem", "Stack", "link");
    Value *fptr = vgen_func_ptr(ir->alc, func, NULL);
    //
    Array* types = array_make(ir->alc, 2);
    array_push(types, type_ptr);
    array_push(types, type_ptr);
    Array* values = array_make(ir->alc, 2);
    array_push(values, on);
    array_push(values, to);
    Array* args = ir_fcall_ir_args(ir, values, types);
    //
    char* link = ir_value(ir, fptr);
    char* link_rett = ir_func_call(ir, link, args, "ptr", 0, 0);
    ir_jump(ir, after);

    ir->block = after;
    return to;
}
