
#include "../all.h"

char *ir_var(IRFunc* func) {
    char *res = al(func->ir->alc, 10);
    res[0] = '%';
    res[1] = '.';
    itos(func->var_count++, res + 2, 10);
    return res;
}
char* ir_arg_nr(IR* ir, int nr) {
    char buf[64];
    strcpy(buf, "\%arg.");
    itos(nr, buf + 5, 10);
    return dups(ir->alc, buf);
}
void ir_decl_store(IR* ir, Decl* decl, char* val) {
    if(!decl->is_mut){
        decl->ir_var = val;
    }
    if (decl->ir_store) {
        ir_store_old(ir, decl->type, decl->ir_store, val);
    }
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

char *ir_int(IR* ir, v_i64 value) {
    char *res = al(ir->alc, 32);
    v_i64 v = value;
    if (value < 0) {
        res[0] = '-';
        v = v * -1;
    }
    itos(v, res + (value < 0 ? 1 : 0), 10);
    return res;
}
char *ir_float(IR* ir, double value) {
    char *res = al(ir->alc, 64);
    sprintf(res, "%f", value);
    return res;
}

Array *ir_fcall_args(IR *ir, Array *values, Array* rett_refs) {
    Array *ir_values = array_make(ir->alc, values->length + 1);
    Array *types = array_make(ir->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        Value *v = array_get_index(values, i);
        char *val = ir_value(ir, v);
        array_push(ir_values, val);
        array_push(types, v->rett);
    }
    if (rett_refs) {
        for (int i = 0; i < rett_refs->length; i++) {
            Value *v = array_get_index(rett_refs, i);
            char *val = v ? ir_assign_value(ir, v) : "null";
            array_push(ir_values, val);
            array_push(types, type_gen_valk(ir->alc, ir->b, "ptr"));
        }
    }
    return ir_fcall_ir_args(ir, ir_values, types);
}

Array *ir_fcall_ir_args(IR *ir, Array *values, Array* types) {
    Array *result = array_make(ir->alc, values->length + 1);
    for (int i = 0; i < values->length; i++) {
        char *val = array_get_index(values, i);
        Type *type = array_get_index(types, i);

        char *buf = ir->char_buf;
        char *ltype = ir_type(ir, type);

        int len = strlen(ltype);
        bool notnull = type->type == type_struct && type->is_pointer && type->nullable == false;
        //
        strcpy(buf, ltype);
        buf[len++] = ' ';
        if(notnull) {
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
    str_preserve(ir->block->code, 200);

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
    if(values) {
        int argc = values->length;
        for (int i = 0; i < argc; i++) {
            str_preserve(ir->block->code, 200);
            char *lval = array_get_index(values, i);
            if (i > 0) {
                str_flat(code, ", ");
            }
            str_add(code, lval);
        }
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
    ir_define_ext_func(ir, func);
    char buf[512];
    strcpy(buf, "@");
    strcat(buf, func->export_name);
    return dups(ir->alc, buf);
}

char* ir_load(IR* ir, Type* type, char* var) {
    char *var_result = ir_var(ir->func);
    char *ltype = ir_type(ir, type);

    char bytes[20];

    Str *code = ir->block->code;
    str_preserve(code, 200);

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

void ir_store(IR *ir, char *var, char *val, char* type, int type_size) {
    Str *code = ir->block->code;
    str_preserve(code, 512);

    if(!var) {
        build_err(ir->b, "Missing IR store var (compiler bug)");
    }

    char bytes[20];
    if(type_size > ir->b->ptr_size)
        type_size = ir->b->ptr_size;
    itos(type_size, bytes, 10);

    str_flat(code, "  store ");
    str_add(code, type);
    str_flat(code, " ");
    str_add(code, val);
    str_flat(code, ", ptr ");
    str_add(code, var);
    str_flat(code, ", align ");
    str_add(code, bytes);
    str_flat(code, "\n");
}

void ir_store_old(IR *ir, Type *type, char *var, char *val) {
    Str *code = ir->block->code;
    char *ltype = ir_type(ir, type);

    if(!var) {
        build_err(ir->b, "Missing IR store var (compiler bug)");
    }

    str_preserve(code, 512);

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
    char *int_type_for_ptr = ir_type_int(ir, ir->b->ptr_size);

    if (from_type->is_pointer && !to_type->is_pointer) {
        // Ptr to int
        char *int_type = ir_type_int(ir, to_type->size);
        char *var = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, " = ptrtoint ptr ");
        str_add(code, result_var);
        str_flat(code, " to ");
        str_add(code, int_type);
        str_flat(code, "\n");
        result_var = var;

        if(to_type->type == type_float){
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = uitofp ");
            str_add(code, int_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ");
            str_add(code, lto_type);
            str_flat(code, "\n");
            result_var = var;
        }

    } else if (!from_type->is_pointer) {

        bool from_int = from_type->type == type_int || from_type->type == type_bool;
        bool from_float = !from_int;

        bool to_int = to_type->is_pointer || to_type->type == type_int || to_type->type == type_bool;
        bool to_float = !to_int;

        if (from_type->size < to_type->size) {
            // Ext
            char *new_type = from_int ? ir_type_int(ir, to_type->size) : ir_type_float(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = ");
            if (from_int) {
                str_add(code, from_type->is_signed ? "sext" : "zext");
            } else {
                str_add(code, "fpext");
            }
            str_flat(code, " ");
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
            char *new_type = from_int ? ir_type_int(ir, to_type->size) : ir_type_float(ir, to_type->size);
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = ");
            str_add(code, from_int ? "trunc" : "fptrunc");
            str_flat(code, " ");
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
            if (from_float) {
                char *var = ir_var(ir->func);
                str_flat(code, "  ");
                str_add(code, var);
                str_flat(code, " = fptoui ");
                str_add(code, lfrom_type);
                str_flat(code, " ");
                str_add(code, result_var);
                str_flat(code, " to ");
                str_add(code, int_type_for_ptr);
                str_flat(code, "\n");
                result_var = var;
                lfrom_type = int_type_for_ptr;
            }

            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = inttoptr ");
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ptr\n");
            result_var = var;
        } else if (from_int && to_float) {
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = ");
            str_add(code, from_type->is_signed ? "sitofp" : "uitofp");
            str_flat(code, " ");
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ");
            str_add(code, lto_type);
            str_flat(code, "\n");
            result_var = var;
        } else if (from_float && to_int) {
            char *var = ir_var(ir->func);
            str_flat(code, "  ");
            str_add(code, var);
            str_flat(code, " = ");
            str_add(code, to_type->is_signed ? "fptosi" : "fptoui");
            str_flat(code, " ");
            str_add(code, lfrom_type);
            str_flat(code, " ");
            str_add(code, result_var);
            str_flat(code, " to ");
            str_add(code, lto_type);
            str_flat(code, "\n");
            result_var = var;
        }
    }

    return result_var;
}

char *ir_i1_cast(IR *ir, char *val) {
    char *var_i1 = ir_var(ir->func);
    Str* code = ir->block->code;
    str_preserve(code, 200);

    str_flat(code, "  ");
    str_add(code, var_i1);
    str_flat(code, " = trunc i8 ");
    str_add(code, val);
    str_flat(code, " to i1\n");
    return var_i1;
}

char* ir_op(IR* ir, int op, char* left, char* right, Type* rett) {

    bool is_float = rett->type == type_float;
    char *ltype = ir_type(ir, rett);
    char *var = ir_var(ir->func);
    Str *code = ir->block->code;

    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = ");
    if (op == op_add) {
        str_add(code, is_float ? "fadd " : "add ");
    } else if (op == op_sub) {
        str_add(code, is_float ? "fsub " : "sub ");
    } else if (op == op_mul) {
        str_add(code, is_float ? "fmul " : "mul ");
    } else if (op == op_div) {
        if (is_float) {
            str_flat(code, "fdiv ");
        } else if (rett->is_signed) {
            str_flat(code, "sdiv ");
        } else {
            str_flat(code, "udiv ");
        }
    } else if (op == op_mod) {
        if (is_float) {
            str_flat(code, "frem ");
        } else if (rett->is_signed) {
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

char* ir_compare(IR* ir, int op, char* left, char* right, char* type, bool is_signed, bool is_float) {

    char *var_result = ir_var(ir->func);

    char *sign = is_float ? "oeq" : "eq";
    if (op == op_ne) {
        sign = is_float ? "one" : "ne";
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
    str_add(code, var_result);
    str_add(code, is_float ? " = fcmp " : " = icmp ");
    str_add(code, sign);
    str_flat(code, " ");
    str_add(code, type);
    str_flat(code, " ");
    str_add(code, left);
    str_flat(code, ", ");
    str_add(code, right);
    str_flat(code, "\n");

    return var_result;
}

char *ir_class_pa(IR *ir, Class *class, char *on, ClassProp *prop) {

    char *result = ir_var(ir->func);
    Str *code = ir->block->code;
    str_preserve(code, 512);

    char index[20];
    itos(prop->index, index, 10);

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

void ir_if(IR *ir, TIf *ift) {
    //
    Value *cond = ift->cond;
    Scope *scope_if = ift->scope_if;
    Scope *scope_else = ift->scope_else;

    IRBlock *block_if = ir_block_make(ir, ir->func, "if_");
    IRBlock *block_else = ir_block_make(ir, ir->func, "if_else_");
    IRBlock *after = ir_block_make(ir, ir->func, "if_after_");

    char *lcond = ir_value(ir, cond);
    ir_cond_jump(ir, lcond, block_if, block_else);

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

void ir_while(IR *ir, TWhile *item) {
    //
    Value *cond = item->cond;
    Scope *scope_while = item->scope_while;

    IRBlock *block_cond = ir_block_make(ir, ir->func, "while_cond_");
    IRBlock *block_while = ir_block_make(ir, ir->func, "while_code_");
    IRBlock *after = ir_block_make(ir, ir->func, "while_after_");

    ir_jump(ir, block_cond);

    ir->block = block_cond;
    char *lcond = ir_value(ir, cond);
    ir_cond_jump(ir, lcond, block_while, after);

    // Code
    ir->block = block_while;

    // Ast
    IRBlock* backup_after = ir->block_after;
    IRBlock* backup_cond = ir->block_cond;
    ir->block_after = after;
    ir->block_cond = block_cond;
    ir_write_ast(ir, scope_while);
    ir->block_after = backup_after;
    ir->block_cond = backup_cond;


    if (!scope_while->did_return) {
        ir_jump(ir, block_cond);
    }

    //
    ir->block = after;
}

char* ir_ptrv(IR* ir, char* on, char* type, int index) {

    char index_buf[20];
    itos(index, index_buf, 10);

    char *result = ir_var(ir->func);
    Str *code = ir->block->code;
    str_preserve(code, 250);

    str_flat(code, "  ");
    str_add(code, result);
    str_flat(code, " = getelementptr inbounds ");
    str_add(code, type);
    str_flat(code, ", ptr ");
    str_add(code, on);
    str_flat(code, ", i32 ");
    str_add(code, index_buf);
    str_flat(code, "\n");

    return result;
}
char* ir_ptr_offset(IR* ir, char* on, char* index, char* index_type, int size) {

    char size_buf[20];
    itos(size, size_buf, 10);

    char *result = ir_var(ir->func);
    Str *code = ir->block->code;
    str_preserve(code, 250);

    str_flat(code, "  ");
    str_add(code, result);
    str_flat(code, " = getelementptr [");
    str_add(code, size_buf);
    str_flat(code, " x i8], ptr ");
    str_add(code, on);
    str_flat(code, ", ");
    str_add(code, index_type);
    str_flat(code, " ");
    str_add(code, index);
    str_flat(code, "\n");

    return result;
}
char* ir_ptrv_dyn(IR* ir, char* on, char* type, char* index, char* index_type) {

    char *result = ir_var(ir->func);
    Str *code = ir->block->code;
    str_preserve(code, 200);

    str_flat(code, "  ");
    str_add(code, result);
    str_flat(code, " = getelementptr inbounds ");
    str_add(code, type);
    str_flat(code, ", ptr ");
    str_add(code, on);
    str_flat(code, ", ");
    str_add(code, index_type);
    str_flat(code, " ");
    str_add(code, index);
    str_flat(code, "\n");

    return result;
}

char* ir_this_or_that(IR* ir, char* this, IRBlock* this_block, char* that, IRBlock* that_block, char* type) {
    Str *code = ir->block->code;
    char *var = ir_var(ir->func);
    str_preserve(code, 200);

    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = phi ");
    str_add(code, type);
    str_flat(code, " [ ");
    str_add(code, this);
    str_flat(code, ", %");
    str_add(code, this_block->name);
    str_flat(code, " ], [ ");
    str_add(code, that);
    str_flat(code, ", %");
    str_add(code, that_block->name);
    str_flat(code, " ]\n");

    return var;
}

char* ir_this_or_that_or_that(IR* ir, char* this, IRBlock* this_block, char* that, IRBlock* that_block, char* that2, IRBlock* that_block2, char* type) {
    Str *code = ir->block->code;
    char *var = ir_var(ir->func);
    str_preserve(code, 200);

    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = phi ");
    str_add(code, type);
    str_flat(code, " [ ");
    str_add(code, this);
    str_flat(code, ", %");
    str_add(code, this_block->name);
    str_flat(code, " ], [ ");
    str_add(code, that);
    str_flat(code, ", %");
    str_add(code, that_block->name);
    str_flat(code, " ], [ ");
    str_add(code, that2);
    str_flat(code, ", %");
    str_add(code, that_block2->name);
    str_flat(code, " ]\n");

    return var;
}

char* ir_phi(IR* ir, Array* values, char* type) {
    Str *code = ir->block->code;
    char *var = ir_var(ir->func);

    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = phi ");
    str_add(code, type);

    for(int i = 0; i < values->length; i ++){
        str_preserve(code, 200);

        IRPhiValue* v = array_get_index(values, i);

        if (i > 0)
            str_flat(code, ",");
        str_flat(code, " [ ");
        str_add(code, v->value);
        str_flat(code, ", %");
        str_add(code, v->block->name);
        str_flat(code, " ]");
    }

    str_flat(code, "\n");

    return var;
}

char *ir_notnull_i1(IR *ir, char *val) {
    char *var = ir_var(ir->func);
    Str *code = ir->block->code;
    str_preserve(code, 200);

    str_flat(code, "  ");
    str_add(code, var);
    str_flat(code, " = icmp ne ptr ");
    str_add(code, val);
    str_flat(code, ", null\n");
    return var;
}

char *ir_and_or(IR *ir, IRBlock* block_current, char *left, IRBlock* block_right, char* right, IRBlock* block_last, int op) {

    bool is_or = op == op_or;

    IRBlock *block_after = ir_block_make(ir, ir->func, "and_or_after_");

    if (is_or) {
        ir_cond_jump(ir, left, block_after, block_right);
    } else {
        ir_cond_jump(ir, left, block_right, block_after);
    }

    ir->block = block_last;
    ir_jump(ir, block_after);

    ir->block = block_after;
    return ir_this_or_that(ir, is_or ? "true" : "false", block_current, right, block_last, "i1");
}