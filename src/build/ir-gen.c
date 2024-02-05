
#include "../all.h"

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    sprintf(res, "%%.%d", func->var_count++);
    return res;
}

void ir_jump(Str* code, IRBlock* block) {
    str_append_chars(code, "  br label %");
    str_append_chars(code, block->name);
    str_append_chars(code, "\n");
}

Array *ir_fcall_args(IR *ir, Scope *scope, Array *values) {
    Array *result = array_make(ir->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        Value *v = array_get_index(values, i);
        char *val = ir_value(ir, scope, v);
        char *buf = ir->char_buf;
        sprintf(buf, "%s noundef%s %s", ir_type(ir, v->rett), !v->rett->is_pointer || v->rett->nullable ? "" : " nonnull", val);
        array_push(result, dups(ir->alc, buf));
    }
    return result;
}

char *ir_func_call(IR *ir, char *on, Array *values, char *lrett, int line, int col) {
    Str *code = ir->block->code;
    char *var_result = "";
    str_append_chars(code, "  ");
    if (strcmp(lrett, "void") != 0) {
        var_result = ir_var(ir->func);
        str_append_chars(code, var_result);
        str_append_chars(code, " = ");
    }
    str_append_chars(code, "call ");
    str_append_chars(code, lrett);
    str_append_chars(code, " ");
    str_append_chars(code, on);
    str_append_chars(code, "(");
    int argc = values->length;
    for (int i = 0; i < values->length; i++) {
        char *lval = array_get_index(values, i);
        if (i > 0) {
            str_append_chars(code, ", ");
        }
        str_append_chars(code, lval);
    }
    str_append_chars(code, ")");
    // if (ir->debug) {
    //     char *loc = ir_attr(b);
    //     str_append_chars(code, ", !dbg ");
    //     str_append_chars(code, loc);
    //     sprintf(b->char_buf, "%s = !DILocation(line: %d, column: %d, scope: %s)", loc, line, col, b->lfunc->di_scope);
    //     array_push(b->attrs, dups(b->alc, b->char_buf));
    // }
    str_append_chars(code, "\n");
    return var_result;
}

char *ir_func_ptr(IR *ir, Func *func) {
    //
    if (func->fc != ir->fc) {
        // Extern function
        ir_define_ext_func(ir, func);
    }
    char buf[512];
    strcpy(buf, "@");
    strcat(buf, func->export_name);
    return dups(ir->alc, buf);
}

char *ir_string(IR *ir, char *body) {
    //
    Fc *fc = ir->fc;
    Str *code = ir->code_global;

    sprintf(ir->char_buf, "@.str.%d", ir->string_count++);
    char *var = dups(ir->alc, ir->char_buf);

    int ptr_size = ir->b->ptr_size;
    int len = strlen(body);
    int blen = len + ptr_size + 2;

    sprintf(ir->char_buf, "%s = private unnamed_addr constant [%d x i8] c\"", var, blen);
    str_append_chars(code, ir->char_buf);

    // Bytes
    // Len bytes
    size_t len_buf = len;
    unsigned char *len_ptr = (unsigned char *)&len_buf;
    int c = 0;
    while (c < ptr_size) {
        unsigned char ch = *(len_ptr + c);
        c++;
        str_append_char(code, '\\');
        char hex[20];
        sprintf(hex, "%02X", ch);
        if (strlen(hex) == 0) {
            str_append_char(code, '0');
        }
        str_append_chars(code, hex);
    }

    // Const byte
    str_append_chars(code, "\\01");
    str_append_chars(code, "\\00");

    // String bytes
    int index = 0;
    while (index < len) {
        unsigned char ch = body[index];
        index++;
        if (ch > 34 && ch < 127 && ch != 92) {
            str_append_char(code, ch);
            continue;
        }
        str_append_char(code, '\\');
        char hex[20];
        sprintf(hex, "%02X", ch);
        if (strlen(hex) == 0) {
            str_append_char(code, '0');
        }
        str_append_chars(code, hex);
    }
    //
    str_append_chars(code, "\", align 8\n");

    sprintf(ir->char_buf, "getelementptr inbounds ([%d x i8], [%d x i8]* %s, i64 0, i64 0)", blen, blen, var);
    return dups(ir->alc, ir->char_buf);
}