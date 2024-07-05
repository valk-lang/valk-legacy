
#include "../all.h"

void ir_gen_globals(IR* ir) {
    //
    Unit *u = ir->unit;
    Build *b = ir->b;
    Str *code = ir->code_global;

    if (u->is_main) {
        str_preserve(code, 500);
        str_flat(code, "@valk_err_code = dso_local thread_local(initialexec) global i32 0, align 4\n");
        str_flat(code, "@valk_err_msg = dso_local thread_local(initialexec) global i8* null, align 8\n");
    } else {
        str_preserve(code, 500);
        str_flat(code, "@valk_err_code = external thread_local(initialexec) global i32, align 4\n");
        str_flat(code, "@valk_err_msg = external thread_local(initialexec) global i8*, align 8\n");
        str_flat(code, "@valk_gc_vtable = external constant ptr, align 8\n");
    }

    loop(u->globals, i) {
        Global *g = array_get_index(u->globals, i);
        if (g->fc && g->fc->is_header)
            continue;
        if (g->fc && g->fc->is_header)
            continue;

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
    if (g->unit != ir->unit && !array_contains(ir->declared_globals, g, arr_find_adr)) {
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

char *ir_string(IR *ir, VString *str) {
    //
    if(array_contains(ir->declared_globals, str, arr_find_adr))
        return ir_ptrv(ir, str->ir_object_name, "ptr", 1);

    array_push(ir->declared_globals, str);

    Str *code = ir->code_global;

    char* body_name = str->ir_body_name;
    char* object_name = str->ir_object_name;
    char* body = str->body;

    int ptr_size = ir->b->ptr_size;
    int len = strlen(body);
    int blen = len + 1;

    char blen_str[32];
    itos(blen, blen_str, 10);
    char len_str[32];
    itos(len, len_str, 10);

    Type* stype = type_gen_valk(ir->alc, ir->b, "String");

    char vt[32];
    itos(stype->class->gc_vtable_index, vt, 10);

    char gcpt[32];
    itos(get_valk_class(ir->b, "mem", "GcPtr")->gc_vtable_index, gcpt, 10);

    char body_type[512];
    strcpy(body_type, body_name);
    body_type[0] = '%';

    bool external = false;

    str_add(code, body_type);
    str_flat(code, " = type <{ i8, i8, i8, i8, i32, i64, [");
    str_add(code, blen_str);
    str_flat(code, " x i8] }>\n");

    // Define object global
    str_preserve(code, 512);
    str_add(code, object_name);
    str_flat(code, " = ");
    str_add(code, external ? "external" : "dso_local");
    str_flat(code, " global ");
    str_add(code, body_type);
    if (!external) {
        str_flat(code, " <{ i8 8, i8 0, i8 0, i8 0, i32 ");
        str_add(code, vt);
        str_flat(code, ", ");
        str_add(code, ir_type_int(ir, ir->b->ptr_size));
        str_flat(code, " ");
        str_add(code, len_str);
        str_flat(code, ", ");
        // Body

        str_flat(code, "[");
        str_add(code, blen_str);
        str_flat(code, " x i8] c\"");
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
            char hex[3];
            char_to_hex(ch, hex);
            ((char *)code->data)[code->length] = hex[0];
            ((char *)code->data)[code->length + 1] = hex[1];
            code->length += 2;
        }
        str_flat(code, "\\00\""); // 0 byte to end string

        // End body
        str_flat(code, " }>");
    }
    str_flat(code, ", align 8\n");

    return ir_ptrv(ir, object_name, "ptr", 1);
}
