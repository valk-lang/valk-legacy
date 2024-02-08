
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
                ir_store(ir, decl->type, decl->ir_var, lval);
            } else {
                decl->ir_var = lval;
            }
            continue;
        }
        if (tt == t_assign) {
            VPair* pair = t->item;
            Value* left = pair->left;
            Value* right = pair->right;
            char* var = ir_assign_value(ir, scope, left);
            char* value = ir_value(ir, scope, right);
            ir_store(ir, left->rett, var, value);
            continue;
        }

        if (tt == t_return) {
            Value *v = t->item;
            char *irv = "void";
            if(v)
                irv = ir_value(ir, scope, v);
            Str* code = ir->block->code;
            str_flat(code, "  ret ");
            if(v) {
                str_add(code, ir_type(ir, v->rett));
                str_flat(code, " ");
                str_add(code, irv);
            } else {
                str_flat(code, "void");
            }
            str_flat(code, "\n");
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
