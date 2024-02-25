
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

        if (tt == t_statement) {
            Value *v = t->item;
            char *irv = ir_value(ir, scope, v);
            continue;
        }
        if (tt == t_declare) {
            TDeclare* item = t->item;
            Decl *decl = item->decl;
            Value *val = item->value;

            char *lval = ir_value(ir, scope, val);
            if (decl->is_mut) {
                ir_store_old(ir, decl->type, decl->ir_store_var, lval);
            } else {
                if(decl->is_gc) {
                    ir_store_old(ir, decl->type, decl->ir_store_var, lval);
                }
                decl->ir_var = lval;
            }
            continue;
        }
        if (tt == t_assign) {
            VPair* pair = t->item;
            Value* left = pair->left;
            Value* right = pair->right;
            char* value = ir_value(ir, scope, right);
            if (left->type == v_class_pa && type_is_gc(left->rett) && type_is_gc(right->rett)) {
                // GC link
                VClassPA *pa = left->item;
                Type* on_type = pa->on->rett;
                char* on = ir_value(ir, scope, pa->on);
                char* var = ir_class_pa(ir, on_type->class, on, pa->prop);
                char* result = ir_gc_link(ir, on, value);
                ir_store(ir, var, result, "ptr", ir->b->ptr_size);
            } else {
                char* var = ir_assign_value(ir, scope, left);
                ir_store_old(ir, left->rett, var, value);
            }
            continue;
        }

        if (tt == t_return) {
            Value *v = t->item;
            if(v) {
                char* irv = ir_value(ir, scope, v);
                ir_func_return(ir, ir_type(ir, v->rett), irv);
            } else {
                ir_func_return(ir, NULL, "void");
            }
            continue;
        }

        if (tt == t_if) {
            TIf *ift = t->item;
            ir_if(ir, scope, ift);
            continue;
        }

        if (tt == t_while) {
            TWhile *item = t->item;
            ir_while(ir, scope, item);
            continue;
        }
        if (tt == t_break) {
            Scope* loop_scope = t->item;
            IRBlock* after = loop_scope->ir_after_block;
            if(!after) {
                die("Missing IR after block for 'break' (compiler bug)");
            }
            ir_jump(ir, after);
            continue;
        }
        if (tt == t_continue) {
            Scope* loop_scope = t->item;
            IRBlock* cond = loop_scope->ir_cond_block;
            if(!cond) {
                die("Missing IR condition block for 'break' (compiler bug)");
            }
            ir_jump(ir, cond);
            continue;
        }
        if (tt == t_throw) {
            TThrow* tt = t->item;
            ir_store_old(ir, type_gen_volt(alc, ir->b, "i32"), "@volt_err_code", ir_int(ir, tt->err->value));
            char *msg = ir_string(ir, tt->msg, tt->msg->fc != ir->fc);
            ir_store_old(ir, type_gen_volt(alc, ir->b, "ptr"), "@volt_err_msg", msg);
            ir_func_return_nothing(ir);
            continue;
        }
        if (tt == t_gc_unlink) {
            Value* val = t->item;
            char* ir_val = ir_value(ir, scope, val);
            ir_gc_unlink(ir, ir_val, val->rett->nullable);
            continue;
        }
        if (tt == t_set_var) {
            VVar *vv = t->item;
            vv->var = ir_value(ir, scope, vv->value);
            continue;
        }
        if (tt == t_ast_scope) {
            Scope *s = t->item;
            ir_write_ast(ir, s);
            continue;
        }

        die("Unhandled IR token (compiler bug)");
    }
}

char* ir_gc_unlink(IR* ir, char* val, bool nullable) {
    Build* b = ir->b;

    Type* type_u8 = type_gen_volt(ir->alc, b, "u8");
    Type* type_ptr = type_gen_volt(ir->alc, b, "ptr");

    IRBlock *after = ir_block_make(ir, ir->func, "unlink_after_");
    IRBlock *block_if_min = ir_block_make(ir, ir->func, "unlink_min_state_");
    IRBlock *block_if_max = ir_block_make(ir, ir->func, "unlink_max_state_");

    if(nullable) {
        IRBlock *block_not_null = ir_block_make(ir, ir->func, "unlink_not_null_");

        char* is_null = ir_compare(ir, op_ne, val, "null", "ptr", false, false);
        ir_cond_jump(ir, is_null, block_not_null, after);

        ir->block = block_not_null;
    }

    char* val_state_var = ir_ptrv(ir, val, "i8", 0);
    char* val_state = ir_load(ir, type_u8, val_state_var);

    char* above_new = ir_compare(ir, op_gt, val_state, "0", "i8", false, false);
    ir_cond_jump(ir, above_new, block_if_min, after);

    ir->block = block_if_min;
    char* below_unknown = ir_compare(ir, op_gt, val_state, "6", "i8", false, false);
    ir_cond_jump(ir, below_unknown, block_if_max, after);

    ir->block = block_if_max;
    Func *func = get_volt_class_func(b, "mem", "Stack", "unlink");
    Value *fptr = vgen_func_ptr(ir->alc, func, NULL);
    //
    Array* types = array_make(ir->alc, 2);
    array_push(types, type_ptr);
    Array* values = array_make(ir->alc, 2);
    array_push(values, val);
    Array* args = ir_fcall_ir_args(ir, values, types);
    //
    char* link = ir_value(ir, NULL, fptr);
    char* link_rett = ir_func_call(ir, link, args, "ptr", 0, 0);
    ir_jump(ir, after);

    ir->block = after;
}

char* ir_gc_link(IR* ir, char* on, char* to) {
    Build* b = ir->b;

    Type* type_u8 = type_gen_volt(ir->alc, b, "u8");
    Type* type_ptr = type_gen_volt(ir->alc, b, "ptr");

    char* on_state_var = ir_ptrv(ir, on, "i8", 0);
    char* on_state = ir_load(ir, type_u8, on_state_var);

    IRBlock *current = ir->block;
    IRBlock *block_if = ir_block_make(ir, ir->func, "if_link_");
    IRBlock *block_link = ir_block_make(ir, ir->func, "link_");
    IRBlock *after = ir_block_make(ir, ir->func, "link_after_");

    // On state > transfer
    char *comp_on = ir_compare(ir, op_gt, on_state, "2", "i8", false, false);
    ir_cond_jump(ir, comp_on, block_if, after);

    // To state < solid
    ir->block = block_if;
    char* to_state_var = ir_ptrv(ir, to, "i8", 0);
    char* to_state = ir_load(ir, type_u8, to_state_var);

    char *comp_to = ir_compare(ir, op_lt, to_state, "4", "i8", false, false);
    ir_cond_jump(ir, comp_to, block_link, after);

    // Link
    ir->block = block_link;
    Func *func = get_volt_class_func(b, "mem", "Stack", "link");
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
    char* link = ir_value(ir, NULL, fptr);
    char* link_rett = ir_func_call(ir, link, args, "ptr", 0, 0);
    ir_jump(ir, after);

    ir->block = after;
    char *result = ir_this_or_that_or_that(ir, to, current, to, block_if, link_rett, block_link, "ptr");
    return result;
}
