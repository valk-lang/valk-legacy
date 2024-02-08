
#include "../all.h"

void ir_gen_globals(IR* ir) {
    //
    Fc *fc = ir->fc;
    Scope *scope = fc->scope;
    Str *code = ir->code_global;

    bool is_main_fc = fc->contains_main_func;

    char bytes[20];
    itoa(fc->b->ptr_size, bytes, 10);

    if (is_main_fc) {
        str_flat(code, "@ki_err_code_buffer = dso_local thread_local(initialexec) global i32 0, align 4\n");
        str_flat(code, "@ki_err_msg_buffer = dso_local thread_local(initialexec) global i8* null, align ");
        str_add(code, bytes);
        str_flat(code, "\n");
    } else {
        str_flat(code, "@ki_err_code_buffer = external thread_local(initialexec) global i32, align 4\n");
        str_flat(code, "@ki_err_msg_buffer = external thread_local(initialexec) global i8*, align ");
        str_add(code, bytes);
        str_flat(code, "\n");
    }

    for (int i = 0; i < fc->globals->length; i++) {
        Global *g = array_get_index(fc->globals, i);

        char *name = g->export_name;
        Type *type = g->type;

        char *ltype = ir_type(ir, type);
        str_flat(code, "@");
        str_add(code, name);
        str_flat(code, " = dso_local ");
        str_add(code, g->is_shared ? "" : "thread_local(initialexec) ");
        str_flat(code, "global ");
        str_add(code, ltype);
        str_flat(code, " ");
        if (type->is_pointer) {
            str_flat(code, "null");
        } else {
            str_flat(code, "0");
        }

        char bytes[20];

        str_flat(code, ", align ");
        str_add(code, ir_type_align(ir, type, bytes));
        str_flat(code, "\n");

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
        str_flat(code, "@");
        str_add(code, name);
        str_flat(code, " = external ");
        str_add(code, g->is_shared ? "" : "thread_local(initialexec) ");
        str_flat(code, "global ");
        // str_flat(code, " = external global ");
        str_add(code, ltype);
        str_flat(code, ", align ");
        str_add(code, ir_type_align(ir, type, bytes));
        str_flat(code, "\n");

        array_push(ir->declared_globals, g);
    }

    char* ir_name = al(ir->alc, strlen(name) + 2);
    strcpy(ir_name, "@");
    strcat(ir_name, name);

    return ir_name;
}