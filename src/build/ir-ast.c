
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
            char* var = ir_assign_value(ir, scope, left);
            ir_store(ir, left->rett, var, value);
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
