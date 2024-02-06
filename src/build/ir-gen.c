
#include "../all.h"

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    sprintf(res, "%%.%d", func->var_count++);
    return res;
}

void ir_jump(IR* ir, IRBlock* block) {
    Str* code = ir->block->code;
    str_append_chars(code, "  br label %");
    str_append_chars(code, block->name);
    str_append_chars(code, "\n");
}
void ir_cond_jump(IR* ir, char* cond, IRBlock* block_if, IRBlock* block_else) {
    Str* code = ir->block->code;
    str_append_chars(code, "  br i1 ");
    str_append_chars(code, cond);
    str_append_chars(code, ", label %");
    str_append_chars(code, block_if->name);
    str_append_chars(code, ", label %");
    str_append_chars(code, block_else->name);
    str_append_chars(code, "\n");
}

char *ir_int(IR* ir, long int value) {
    char *res = al(ir->alc, 20);
    sprintf(res, "%ld", value);
    return res;
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

char* ir_load(IR* ir, Type* type, char* var) {
    Str *code = ir->block->code;
    char *var_result = ir_var(ir->func);
    char *ltype = ir_type(ir, type);

    char bytes[20];
    int abytes = type->size;
    if (abytes > ir->b->ptr_size) {
        abytes = ir->b->ptr_size;
    }
    sprintf(bytes, "%d", abytes);

    str_append_chars(code, "  ");
    str_append_chars(code, var_result);
    str_append_chars(code, " = load ");
    str_append_chars(code, ltype);
    str_append_chars(code, ", ptr ");
    str_append_chars(code, var);
    str_append_chars(code, ", align ");
    str_append_chars(code, bytes);
    str_append_chars(code, "\n");

    return var_result;
}
void ir_store(IR *ir, Type *type, char *var, char *val) {
    Str *code = ir->block->code;
    char *ltype = ir_type(ir, type);

    char bytes[20];
    ir_type_align(ir, type, bytes);

    str_append_chars(code, "  store ");
    str_append_chars(code, ltype);
    str_append_chars(code, " ");
    str_append_chars(code, val);
    str_append_chars(code, ", ptr ");
    str_append_chars(code, var);
    str_append_chars(code, ", align ");
    str_append_chars(code, bytes);
    str_append_chars(code, "\n");
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
        str_append_chars(code, "  ");
        str_append_chars(code, var);
        str_append_chars(code, " = ptrtoint ");
        str_append_chars(code, lfrom_type);
        str_append_chars(code, " ");
        str_append_chars(code, result_var);
        str_append_chars(code, " to ");
        str_append_chars(code, lto_type);
        str_append_chars(code, "\n");
        result_var = var;
    } else if (!from_type->is_pointer) {
        if (from_type->size < to_type->size) {
            // Ext
            char *new_type = ir_type_int(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_append_chars(code, "  ");
            str_append_chars(code, var);
            if (from_type->is_signed) {
                str_append_chars(code, " = sext ");
            } else {
                str_append_chars(code, " = zext ");
            }
            str_append_chars(code, lfrom_type);
            str_append_chars(code, " ");
            str_append_chars(code, result_var);
            str_append_chars(code, " to ");
            str_append_chars(code, new_type);
            str_append_chars(code, "\n");
            lfrom_type = new_type;
            result_var = var;
        } else if (from_type->size > to_type->size) {
            // Trunc
            char *new_type = ir_type_int(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_append_chars(code, "  ");
            str_append_chars(code, var);
            str_append_chars(code, " = trunc ");
            str_append_chars(code, lfrom_type);
            str_append_chars(code, " ");
            str_append_chars(code, result_var);
            str_append_chars(code, " to ");
            str_append_chars(code, new_type);
            str_append_chars(code, "\n");
            lfrom_type = new_type;
            result_var = var;
        }
        if (to_type->is_pointer) {
            // Bitcast to i8*|%struct...*
            char *var = ir_var(ir->func);
            str_append_chars(code, "  ");
            str_append_chars(code, var);
            str_append_chars(code, " = inttoptr ");
            str_append_chars(code, lfrom_type);
            str_append_chars(code, " ");
            str_append_chars(code, result_var);
            str_append_chars(code, " to ptr\n");
            result_var = var;
        }
    }

    return result_var;
}

char *ir_i1_cast(IR *ir, char *val) {
    char *var_i1 = ir_var(ir->func);
    Str* code = ir->block->code;
    str_append_chars(code, "  ");
    str_append_chars(code, var_i1);
    str_append_chars(code, " = trunc i8 ");
    str_append_chars(code, val);
    str_append_chars(code, " to i1\n");
    return var_i1;
}

char* ir_op(IR* ir, Scope* scope, int op, char* left, char* right, Type* rett) {

    char *ltype = ir_type(ir, rett);
    char *var = ir_var(ir->func);

    Str *code = ir->block->code;
    str_append_chars(code, "  ");
    str_append_chars(code, var);
    str_append_chars(code, " = ");
    if (op == op_add) {
        str_append_chars(code, "add ");
    } else if (op == op_sub) {
        str_append_chars(code, "sub ");
    } else if (op == op_mul) {
        str_append_chars(code, "mul ");
    } else if (op == op_div) {
        if (rett->is_signed) {
            str_append_chars(code, "sdiv ");
        } else {
            str_append_chars(code, "udiv ");
        }
    } else if (op == op_mod) {
        if (rett->is_signed) {
            str_append_chars(code, "srem ");
        } else {
            str_append_chars(code, "urem ");
        }
    } else if (op == op_bit_and) {
        str_append_chars(code, "and ");
    } else if (op == op_bit_or) {
        str_append_chars(code, "or ");
    } else if (op == op_bit_xor) {
        str_append_chars(code, "xor ");
    } else if (op == op_shl) {
        str_append_chars(code, "shl ");
    } else if (op == op_shr) {
        str_append_chars(code, "lshr ");
    } else {
        die("Unknown IR math operation (compiler bug)");
    }
    str_append_chars(code, ltype);
    str_append_chars(code, " ");
    str_append_chars(code, left);
    str_append_chars(code, ", ");
    str_append_chars(code, right);
    str_append_chars(code, "\n");

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
    str_append_chars(code, "  ");
    str_append_chars(code, var_tmp);
    str_append_chars(code, " = icmp ");
    str_append_chars(code, sign);
    str_append_chars(code, " ");
    str_append_chars(code, ltype);
    str_append_chars(code, " ");
    str_append_chars(code, lval1);
    str_append_chars(code, ", ");
    str_append_chars(code, lval2);
    str_append_chars(code, "\n");

    str_append_chars(code, "  ");
    str_append_chars(code, var_result);
    str_append_chars(code, " = zext i1 ");
    str_append_chars(code, var_tmp);
    str_append_chars(code, " to i8\n");

    return var_result;
}

char *ir_class_pa(IR *ir, Class *class, char *on, ClassProp *prop) {
    char *result = ir_var(ir->func);
    Str *code = ir->block->code;

    char index[20];
    sprintf(index, "%d", prop->index);

    ir_define_struct(ir, class);

    str_append_chars(code, "  ");
    str_append_chars(code, result);
    str_append_chars(code, " = getelementptr inbounds %struct.");
    str_append_chars(code, class->ir_name);
    str_append_chars(code, ", ptr ");
    str_append_chars(code, on);
    str_append_chars(code, ", i32 0, i32 ");
    str_append_chars(code, index);
    str_append_chars(code, "\n");

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
    ir_cond_jump(ir, lcond_i1, block_while, after);

    ir->block = block_while;
    ir_write_ast(ir, scope_while);
    if (!scope_while->did_return) {
        ir_jump(ir, block_cond);
    }

    ir->block = after;
}
