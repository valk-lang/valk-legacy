
#include "../all.h"

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    res[0] = '%';
    res[1] = '.';
    itoa(func->var_count++, res + 2, 10);
    return res;
}

void ir_jump(IR* ir, IRBlock* block) {
    Str* code = ir->block->code;
    str_flat(code, "  br label %");
    str_add(code, block->name);
    str_flat(code, "\n");
}
void ir_cond_jump(IR* ir, char* cond, IRBlock* block_if, IRBlock* block_else) {
    Str* code = ir->block->code;
    str_flat(code, "  br i1 ");
    str_add(code, cond);
    str_flat(code, ", label %");
    str_add(code, block_if->name);
    str_flat(code, ", label %");
    str_add(code, block_else->name);
    str_flat(code, "\n");
}

char *ir_int(IR* ir, long int value) {
    char *res = al(ir->alc, 24);
    itoa(value, res, 10);
    return res;
}

Array *ir_fcall_args(IR *ir, Scope *scope, Array *values) {
    Array *result = array_make(ir->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        Value *v = array_get_index(values, i);
        char *val = ir_value(ir, scope, v);
        char *buf = ir->char_buf;

        char *type = ir_type(ir, v->rett);
        int len = strlen(type);
        bool nullable = !v->rett->is_pointer || v->rett->nullable;
        //
        strcpy(buf, type);
        buf[len++] = ' ';
        if(!nullable) {
            strcpy(buf + len, "nonnull ");
            strcpy(buf + len + 8, val);
        } else {
            strcpy(buf + len, val);
        }
        array_push(result, dups(ir->alc, buf));
    }
    return result;
}

char *ir_func_call(IR *ir, char *on, Array *values, char *lrett, int line, int col) {
    Str *code = ir->block->code;
    char *var_result = "";
    str_flat(code, "  ");
    if (!str_is(lrett, "void")) {
        var_result = ir_var(ir->func);
        str_add(code, var_result);
        str_flat(code, " = ");
    }
    str_flat(code, "call ");
    str_add(code, lrett);
    str_flat(code, " ");
    str_add(code, on);
    str_flat(code, "(");
    int argc = values->length;
    for (int i = 0; i < values->length; i++) {
        char *lval = array_get_index(values, i);
        if (i > 0) {
            str_flat(code, ", ");
        }
        str_add(code, lval);
    }
    str_flat(code, ")");
    // if (ir->debug) {
    //     char *loc = ir_attr(b);
    //     str_flat(code, ", !dbg ");
    //     str_add(code, loc);
    //     sprintf(b->char_buf, "%s = !DILocation(line: %d, column: %d, scope: %s)", loc, line, col, b->lfunc->di_scope);
    //     array_push(b->attrs, dups(b->alc, b->char_buf));
    // }
    str_flat(code, "\n");
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
    str_preserve(code, 100);

    char var[64];
    strcpy(var, "@.str.");
    itoa(ir->string_count++, (char*)((intptr_t)var + 6), 10);

    int ptr_size = ir->b->ptr_size;
    int len = strlen(body);
    int blen = len + ptr_size + 2;

    str_add(code, var);
    str_flat(code, " = private unnamed_addr constant [");
    char len_str[32];
    itoa(blen, len_str, 10);
    str_add(code, len_str);
    str_flat(code, " x i8] c\"");

    // Bytes
    // Len bytes
    size_t len_buf = len;
    unsigned char *len_ptr = (unsigned char *)&len_buf;
    int c = 0;
    while (c++ < ptr_size) {
        //
        unsigned char ch = (c > sizeof(size_t)) ? 0 : *(len_ptr + c - 1);
        //
        ((char*)code->data)[code->length] = '\\';
        code->length++;
        //
        unsigned char hex[3];
        char_to_hex(ch, hex);
        ((char *)code->data)[code->length] = hex[0];
        ((char *)code->data)[code->length + 1] = hex[1];
        code->length += 2;
    }

    // Const byte
    str_flat(code, "\\01");
    str_flat(code, "\\00");

    // String bytes
    int index = 0;
    while (index < len) {
        if(index % 100 == 0)
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
    str_flat(code, "\", align 8\n");

    sprintf(ir->char_buf, "getelementptr inbounds ([%d x i8], [%d x i8]* %s, i64 0, i64 0)", blen, blen, var);
    return dups(ir->alc, ir->char_buf);
}

char* ir_load(IR* ir, Type* type, char* var) {
    char *var_result = ir_var(ir->func);
    char *ltype = ir_type(ir, type);

    char bytes[20];

    Str *code = ir->block->code;
    str_flat(code, "  ");
    str_add(code, var_result);
    str_flat(code, " = load ");
    str_add(code, ltype);
    str_flat(code, ", ptr ");
    str_add(code, var);
    str_flat(code, ", align ");
    str_add(code, ir_type_align(ir, type, bytes));
    str_flat(code, "\n");

    return var_result;
}
void ir_store(IR *ir, Type *type, char *var, char *val) {
    Str *code = ir->block->code;
    char *ltype = ir_type(ir, type);

    char bytes[20];
    ir_type_align(ir, type, bytes);

    str_flat(code, "  store ");
    str_add(code, ltype);
    str_flat(code, " ");
    str_add(code, val);
    str_flat(code, ", ptr ");
    str_add(code, var);
    str_flat(code, ", align ");
    str_add(code, bytes);
    str_flat(code, "\n");
}

char *ir_cast(IR *ir, char *lval, Type *from_type, Type *to_type) {
    //
    Str *code = ir->block->code;

    char *lfrom_type = ir_type(ir, from_type);
    char *lto_type = ir_type(ir, to_type);
    char *result_var = lval;

    if (from_type->is_pointer && !to_type->is_pointer) {
        // Ptr to int
        char *var = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, " = ptrtoint ");
        str_add(code, lfrom_type);
        str_flat(code, " ");
        str_add(code, result_var);
        str_flat(code, " to ");
        str_add(code, lto_type);
        str_flat(code, "\n");
        result_var = var;
    } else if (!from_type->is_pointer) {
        if (from_type->size < to_type->size) {
            // Ext
            char *new_type = ir_type_int(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            if (from_type->is_signed) {
                str_flat(code, " = sext ");
            } else {
                str_flat(code, " = zext ");
            }
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ");
            str_add(code, new_type);
            str_flat(code, "\n");
            lfrom_type = new_type;
            result_var = var;
        } else if (from_type->size > to_type->size) {
            // Trunc
            char *new_type = ir_type_int(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = trunc ");
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ");
            str_add(code, new_type);
            str_flat(code, "\n");
            lfrom_type = new_type;
            result_var = var;
        }
        if (to_type->is_pointer) {
            // Bitcast to i8*|%struct...*
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = inttoptr ");
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ptr\n");
            result_var = var;
        }
    }

    return result_var;
}

char *ir_i1_cast(IR *ir, char *val) {
    char *var_i1 = ir_var(ir->func);
    Str* code = ir->block->code;
    str_flat(code, "  ");
    str_add(code, var_i1);
    str_flat(code, " = trunc i8 ");
    str_add(code, val);
    str_flat(code, " to i1\n");
    return var_i1;
}

char* ir_op(IR* ir, Scope* scope, int op, char* left, char* right, Type* rett) {

    char *ltype = ir_type(ir, rett);
    char *var = ir_var(ir->func);

    Str *code = ir->block->code;
    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = ");
    if (op == op_add) {
        str_flat(code, "add ");
    } else if (op == op_sub) {
        str_flat(code, "sub ");
    } else if (op == op_mul) {
        str_flat(code, "mul ");
    } else if (op == op_div) {
        if (rett->is_signed) {
            str_flat(code, "sdiv ");
        } else {
            str_flat(code, "udiv ");
        }
    } else if (op == op_mod) {
        if (rett->is_signed) {
            str_flat(code, "srem ");
        } else {
            str_flat(code, "urem ");
        }
    } else if (op == op_bit_and) {
        str_flat(code, "and ");
    } else if (op == op_bit_or) {
        str_flat(code, "or ");
    } else if (op == op_bit_xor) {
        str_flat(code, "xor ");
    } else if (op == op_shl) {
        str_flat(code, "shl ");
    } else if (op == op_shr) {
        str_flat(code, "lshr ");
    } else {
        die("Unknown IR math operation (compiler bug)");
    }
    str_add(code, ltype);
    str_flat(code, " ");
    str_add(code, left);
    str_flat(code, ", ");
    str_add(code, right);
    str_flat(code, "\n");

    return var;
}

char* ir_compare(IR* ir, Scope* scope, int op, Value* left, Value* right) {

    char *lval1 = ir_value(ir, scope, left);
    char *lval2 = ir_value(ir, scope, right);

    Type *type = left->rett;
    char *ltype = ir_type(ir, type);
    bool is_signed = type->is_signed;
    bool is_float = type->type == type_float;

    char *var_tmp = ir_var(ir->func);
    char *var_result = ir_var(ir->func);

    char *sign = "eq";
    if (op == op_ne) {
        sign = "ne";
    } else if (op == op_lt) {
        if (is_float) {
            sign = "olt";
        } else if (is_signed) {
            sign = "slt";
        } else {
            sign = "ult";
        }
    } else if (op == op_lte) {
        if (is_float) {
            sign = "ole";
        } else if (is_signed) {
            sign = "sle";
        } else {
            sign = "ule";
        }
    } else if (op == op_gt) {
        if (is_float) {
            sign = "ogt";
        } else if (is_signed) {
            sign = "sgt";
        } else {
            sign = "ugt";
        }
    } else if (op == op_gte) {
        if (is_float) {
            sign = "oge";
        } else if (is_signed) {
            sign = "sge";
        } else {
            sign = "uge";
        }
    }

    Str *code = ir->block->code;
    str_flat(code, "  ");
    str_add(code, var_tmp);
    str_flat(code, " = icmp ");
    str_add(code, sign);
    str_flat(code, " ");
    str_add(code, ltype);
    str_flat(code, " ");
    str_add(code, lval1);
    str_flat(code, ", ");
    str_add(code, lval2);
    str_flat(code, "\n");

    str_flat(code, "  ");
    str_add(code, var_result);
    str_flat(code, " = zext i1 ");
    str_add(code, var_tmp);
    str_flat(code, " to i8\n");

    return var_result;
}

char *ir_class_pa(IR *ir, Class *class, char *on, ClassProp *prop) {
    char *result = ir_var(ir->func);
    Str *code = ir->block->code;

    char index[20];
    itoa(prop->index, index, 10);

    ir_define_struct(ir, class);

    str_flat(code, "  ");
    str_add(code, result);
    str_flat(code, " = getelementptr inbounds %struct.");
    str_add(code, class->ir_name);
    str_flat(code, ", ptr ");
    str_add(code, on);
    str_flat(code, ", i32 0, i32 ");
    str_add(code, index);
    str_flat(code, "\n");

    return result;
}

void ir_if(IR *ir, Scope *scope, TIf *ift) {
    //
    Value *cond = ift->cond;
    Scope *scope_if = ift->scope_if;
    Scope *scope_else = ift->scope_else;

    IRBlock *block_if = ir_block_make(ir, ir->func);
    IRBlock *block_else = ir_block_make(ir, ir->func);
    IRBlock *after = ir_block_make(ir, ir->func);

    char *lcond = ir_value(ir, scope, cond);
    char *lcond_i1 = ir_i1_cast(ir, lcond);
    ir_cond_jump(ir, lcond_i1, block_if, block_else);

    ir->block = block_if;
    ir_write_ast(ir, scope_if);
    if (!scope_if->did_return) {
        ir_jump(ir, after);
    }

    ir->block = block_else;
    ir_write_ast(ir, scope_else);
    if (!scope_else->did_return) {
        ir_jump(ir, after);
    }

    ir->block = after;
}

void ir_while(IR *ir, Scope *scope, TWhile *item) {
    //
    Value *cond = item->cond;
    Scope *scope_while = item->scope_while;

    IRBlock *block_cond = ir_block_make(ir, ir->func);
    IRBlock *block_while = ir_block_make(ir, ir->func);
    IRBlock *after = ir_block_make(ir, ir->func);

    ir_jump(ir, block_cond);

    ir->block = block_cond;
    char *lcond = ir_value(ir, scope, cond);
    char *lcond_i1 = ir_i1_cast(ir, lcond);
    printf("bcond:%s\n", block_cond->name);
    printf("bwhile:%s\n", block_while->name);
    printf("after:%s\n", after->name);
    printf("cond:%s\n", lcond);
    ir_cond_jump(ir, lcond_i1, block_while, after);

    ir->block = block_while;
    ir_write_ast(ir, scope_while);
    if (!scope_while->did_return) {
        ir_jump(ir, block_cond);
    }

    ir->block = after;
}
