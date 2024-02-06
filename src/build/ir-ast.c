
#include "../all.h"

void ir_write_ast(IR* ir, Scope* scope) {

    Allocator *alc = ir->alc;

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

        if (tt == t_return) {
            Value *v = t->item;
            char *irv = "void";
            if(v)
                irv = ir_value(ir, scope, v);
            Str* code = ir->block->code;
            str_append_chars(code, "  ret ");
            if(v) {
                str_append_chars(code, ir_type(ir, v->rett));
                str_append_chars(code, " ");
                str_append_chars(code, irv);
            } else {
                str_append_chars(code, "void");
            }
            str_append_chars(code, "\n");
            continue;
        }

        die("Unhandled IR token (compiler bug)");
    }
}
