
#include "../all.h"

void ir_write_ast(IR* ir, Scope* scope) {

    Allocator *alc = ir->alc;

    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);

        if (t->type == t_statement) {
            Value *v = t->item;
            char *irv = ir_value(ir, scope, v);
            continue;
        }

        if (t->type == t_return) {
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
