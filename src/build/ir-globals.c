
#include "../all.h"

void ir_gen_globals(IR* ir) {
    //
    Fc *fc = ir->fc;
    Scope *scope = fc->scope;
    Str *code = ir->code_global;

    bool is_main_fc = fc->contains_main_func;

    char bytes[20];
    sprintf(bytes, "%d", fc->b->ptr_size);

    if (is_main_fc) {
        str_append_chars(code, "@ki_err_code_buffer = dso_local thread_local(initialexec) global i32 0, align 4\n");
        str_append_chars(code, "@ki_err_msg_buffer = dso_local thread_local(initialexec) global i8* null, align ");
        str_append_chars(code, bytes);
        str_append_chars(code, "\n");
    } else {
        str_append_chars(code, "@ki_err_code_buffer = external thread_local(initialexec) global i32, align 4\n");
        str_append_chars(code, "@ki_err_msg_buffer = external thread_local(initialexec) global i8*, align ");
        str_append_chars(code, bytes);
        str_append_chars(code, "\n");
    }

    for (int i = 0; i < fc->globals->length; i++) {
        Global *g = array_get_index(fc->globals, i);

        char *name = g->export_name;
        Type *type = g->type;

        char *ltype = ir_type(ir, type);
        str_append_chars(code, "@");
        str_append_chars(code, name);
        str_append_chars(code, " = dso_local ");
        str_append_chars(code, g->is_shared ? "" : "thread_local(initialexec) ");
        str_append_chars(code, " global ");
        str_append_chars(code, ltype);
        str_append_chars(code, " ");
        if (type->is_pointer) {
            str_append_chars(code, "null");
        } else {
            str_append_chars(code, "0");
        }

        char bytes[20];

        str_append_chars(code, ", align ");
        str_append_chars(code, ir_type_align(ir, type, bytes));
        str_append_chars(code, "\n");

        char *val = al(ir->alc, strlen(name) + 2);
        strcpy(val, "@");
        strcat(val, name);

        array_push(ir->declared_globals, g);
    }
}

void *ir_global(IR *ir, Global *g) {
    //
    char *name = g->export_name;
    if (!array_contains(ir->declared_globals, g, arr_find_adr)) {
        Type* type = g->type;
        char *ltype = ir_type(ir, type);

        char bytes[20];

        Str *code = ir->code_global;
        str_append_chars(code, "@");
        str_append_chars(code, name);
        str_append_chars(code, " = external global ");
        str_append_chars(code, ltype);
        str_append_chars(code, ", align ");
        str_append_chars(code, ir_type_align(ir, type, bytes));
        str_append_chars(code, "\n");
    }

    char* ir_name = al(ir->alc, strlen(name) + 2);
    strcpy(ir_name, "@");
    strcat(ir_name, name);

    return ir_name;
}