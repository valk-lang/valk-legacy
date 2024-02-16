
#include "../all.h"

void ir_write_ast(IR* ir, Scope* scope) {

    Allocator *alc = ir->alc;

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
                ir_store(ir, decl->type, decl->ir_store_var, lval);
            } else {
                if(decl->is_gc) {
                    ir_store(ir, decl->type, decl->ir_store_var, lval);
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
                char* on_var = ir_assign_value(ir, scope, pa->on);
                char* on = ir_load(ir, on_type, on_var);
                char* var = ir_class_pa(ir, on_type->class, on, pa->prop);
                ir_gc_link(ir, on, on_type, var, value, pa->prop->type);
            } else {
                char* var = ir_assign_value(ir, scope, left);
                ir_store(ir, left->rett, var, value);
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

        die("Unhandled IR token (compiler bug)");
    }
}


void ir_gc_link(IR* ir, char* on, Type* on_type, char* prop_var, char* to, Type* prop_type) {
    Build* b = ir->b;

    Class* on_class = on_type->class;
    ClassProp* prop_state = class_get_prop(b, on_class, "GC_state");
    Type* ptr_type = type_gen_volt(ir->alc, b, "ptr");

    char *on_state = ir_load(ir, prop_state->type, ir_class_pa(ir, on_class, on, prop_state));

    IRBlock *block_if = ir_block_make(ir, ir->func, "if_link_");
    IRBlock *block_link = ir_block_make(ir, ir->func, "link_");
    IRBlock *dont_link = ir_block_make(ir, ir->func, "dont_link_");
    IRBlock *after = ir_block_make(ir, ir->func, "link_after_");

    // On state > transfer
    char *comp_on = ir_compare(ir, op_gt, on_state, "2", "i8", false, false);
    comp_on = ir_i1_cast(ir, comp_on);
    ir_cond_jump(ir, comp_on, block_if, dont_link);

    // To state < solid
    ir->block = block_if;
    char *to_state = ir_load(ir, prop_state->type, ir_class_pa(ir, prop_type->class, to, prop_state));
    char *comp_to = ir_compare(ir, op_lt, to_state, "4", "i8", false, false);
    comp_to = ir_i1_cast(ir, comp_to);
    ir_cond_jump(ir, comp_to, block_link, dont_link);

    // Link
    ir->block = block_link;
    Func *func = get_volt_class_func(b, "mem", "Stack", "link");
    Value *fptr = vgen_func_ptr(ir->alc, func, NULL);
    //
    Array* types = array_make(ir->alc, 2);
    array_push(types, ptr_type);
    array_push(types, ptr_type);
    Array* values = array_make(ir->alc, 2);
    array_push(values, on);
    array_push(values, to);
    Array* args = ir_fcall_ir_args(ir, values, types);
    //
    char* link = ir_value(ir, NULL, fptr);
    char* link_rett = ir_func_call(ir, link, args, "ptr", 0, 0);
    ir_store(ir, prop_type, prop_var, link_rett);
    ir_jump(ir, after);

    // Dont link
    ir->block = dont_link;
    ir_store(ir, prop_type, prop_var, to);
    ir_jump(ir, after);

    ir->block = after;
}
