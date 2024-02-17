
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
        str_flat(code, "@volt_err_code = dso_local thread_local(initialexec) global i32 0, align 4\n");
        str_flat(code, "@volt_err_msg = dso_local thread_local(initialexec) global i8* null, align ");
        str_add(code, bytes);
        str_flat(code, "\n");
    } else {
        str_flat(code, "@volt_err_code = external thread_local(initialexec) global i32, align 4\n");
        str_flat(code, "@volt_err_msg = external thread_local(initialexec) global i8*, align ");
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

    // Set string globals
    if(is_main_fc) {
        Array *strings = ir->b->strings;
        for (int i = 0; i < strings->length; i++) {
            VString *str = array_get_index(strings, i);
            ir_string(ir, str, false);
        }
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

char *ir_string(IR *ir, VString *str, bool external) {
    //
    if(array_contains(ir->declared_globals, str, arr_find_adr))
        return str->ir_object_name;

    array_push(ir->declared_globals, str);

    Fc *fc = ir->fc;
    Str *code = ir->code_global;

    char* body_name = str->ir_body_name;
    char* object_name = str->ir_object_name;
    char* body = str->body;

    if (!external) {
        int ptr_size = ir->b->ptr_size;
        int len = strlen(body);
        int blen = len + 1;

        str_preserve(code, 200 + blen * 3);

        str_add(code, body_name);
        str_flat(code, " = ");
        str_flat(code, "private unnamed_addr constant [");
        char len_str[32];
        itoa(blen, len_str, 10);
        str_add(code, len_str);
        str_flat(code, " x i8] c\"");

        // String bytes
        int index = 0;
        while (index < len) {
            if (index % 100 == 0)
                str_preserve(code, 100);
            //
            unsigned char ch = body[index++];
            if (ch > 34 && ch < 127 && ch != 92) {
                ((char *)code->data)[code->length] = ch;
                code->length++;
                continue;
            }
            ((char *)code->data)[code->length] = '\\';
            code->length++;
            //
            unsigned char hex[3];
            char_to_hex(ch, hex);
            ((char *)code->data)[code->length] = hex[0];
            ((char *)code->data)[code->length + 1] = hex[1];
            code->length += 2;
        }
        //
        str_flat(code, "\\00"); // 0 byte to end string
        str_flat(code, "\", align 1\n");
    }

    // Define object global
    str_preserve(code, 200);
    Type* t = type_gen_volt(ir->alc, ir->b, "String");
    t->is_pointer = false;
    char *ltype = ir_type(ir, t);
    str_add(code, object_name);
    str_flat(code, " = ");
    str_add(code, external ? "external" : "dso_local");
    str_flat(code, " global ");
    str_add(code, ltype);
    if(!external)
    str_flat(code, " zeroinitializer");
    str_flat(code, ", align 8\n");

    return object_name;
}
