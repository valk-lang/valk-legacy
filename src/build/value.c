
#include "../all.h"

Value* value_handle_idf(Allocator *alc, Fc *fc, Scope *scope, Idf *idf);
Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on);
Value* value_handle_class(Allocator *alc, Fc* fc, Scope* scope, Class* class);
Value* value_handle_ptrv(Allocator *alc, Fc* fc, Scope* scope);
Value* value_handle_compare(Allocator *alc, Fc *fc, Scope *scope, Value *left, Value* right, int op);

Value* read_value(Allocator* alc, Fc* fc, Scope* scope, bool allow_newline, int prio) {
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    char *tkn = tok(fc, true, true, true);
    int t = chunk->token;

    Value* v = NULL;

    if (t == tok_at_word) {
        if (str_is(tkn, "@ptrv")) {
            v = value_handle_ptrv(alc,fc, scope);
        }
    } else if (t == tok_string) {
        char *body = tkn;
        body = string_replace_backslash_chars(alc, body);
        v = value_make(alc, v_string, body, type_gen_volt(alc, b, "string"));
    } else if (t == tok_id) {
        if (str_is(tkn, "sizeof")) {
            tok_expect(fc, "(", false, false);
            Type *type = read_type(fc, alc, scope, true);
            tok_expect(fc, ")", true, true);
            v = vgen_int(alc, type->size, type_gen_volt(alc, b, "int"));
        } else {
            // Identifiers
            Idf *idf = read_idf(fc, scope, tkn, true);
            v = value_handle_idf(alc, fc, scope, idf);
        }
    } else if (t == tok_number || (t == tok_op1 && tok_read_byte(fc, 1) == '-')) {
        bool negative = false;
        if(t == tok_op1) {
            negative = true;
            tkn = tok(fc, true, false, true);
            if(fc->chunk_parse->token != tok_number) {
                sprintf(b->char_buf, "Invalid number: '%s'", tkn);
                parse_err(chunk, b->char_buf);
            }
        }
        char* num = tkn;
        long int iv = 0;
        iv = atoi(num);
        if (negative)
            iv *= -1;
        v = vgen_int(alc, iv, type_gen_volt(alc, b, "int"));
    } else if(t == tok_op2 && (str_is(tkn, "++") || str_is(tkn, "--"))) {
        bool incr = str_is(tkn, "++");
        if(tok_next_is_whitespace(fc)) {
            sprintf(b->char_buf, "Missing value after '%s' (do not use whitespace)", incr ? "++" : "--");
            parse_err(chunk, b->char_buf);
        }
        Value* on = read_value(alc, fc, scope, false, 1);
        if(on->rett->type != type_int) {
            sprintf(b->char_buf, "Invalid value after '%s' (not an integer value)", incr ? "++" : "--");
            parse_err(chunk, b->char_buf);
        }
        if(!value_is_assignable(on)) {
            sprintf(b->char_buf, "Invalid value after '%s' (non-assign-able value)", incr ? "++" : "--");
            parse_err(chunk, b->char_buf);
        }
        value_is_mutable(on);
        v = vgen_incr(alc, b, on, incr, true);
    }

    if(!v) {
        sprintf(b->char_buf, "Unknown value: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }

    ///////////////////////
    // TRAILING CHARS
    ///////////////////////

    t = tok_id_next(fc);
    while(t == tok_char || t == tok_scope_open || t == tok_op2) {
        Type* rett = v->rett;
        char ch = tok_read_byte(fc, t == tok_scope_open ? (sizeof(int) + 1) : 1);
        if(ch == '.') {
            tok(fc, false, false, true);

            Class *class = rett->class;
            if (!class) {
                parse_err(chunk, "Unexpected '.'");
            }

            char* prop_name = tok(fc, false, false, true);
            ClassProp* prop = map_get(class->props, prop_name);
            if(prop) {
                // Property
                v = vgen_class_pa(alc, v, prop);
            } else {
                // Check functions
                Func* func = map_get(class->funcs, prop_name);
                if (!func) {
                    sprintf(b->char_buf, "Unknown class property/function: '%s'", tkn);
                    parse_err(chunk, b->char_buf);
                }
                v = vgen_func_ptr(alc, func, v);
            }
            t = tok_id_next(fc);
            continue;
        }
        if(ch == '(') {
            tok(fc, false, false, true);
            v = value_func_call(alc, fc, scope, v);
            t = tok_id_next(fc);
            continue;
        }
        if((ch == '+' && tok_read_byte(fc, 2) == '+') || (ch == '-' && tok_read_byte(fc, 2) == '-')) {
            tkn = tok(fc, false, false, true);
            bool incr = str_is(tkn, "++");
            if (v->rett->type != type_int) {
                sprintf(b->char_buf, "Invalid value before '%s' (not an integer value)", incr ? "++" : "--");
                parse_err(chunk, b->char_buf);
            }
            if (!value_is_assignable(v)) {
                sprintf(b->char_buf, "Invalid value before '%s' (non-assign-able value)", incr ? "++" : "--");
                parse_err(chunk, b->char_buf);
            }
            value_is_mutable(v);
            v = vgen_incr(alc, b, v, incr, false);
            continue;
        }
        break;
    }

    ///////////////////////
    // TRAILING WORDS & OPS
    ///////////////////////

    tkn = tok(fc, true, true, true);
    while (str_is(tkn, "@as")) {
        if (type_is_void(v->rett)) {
            parse_err(chunk, "Left side of '@as' must return a value");
        }

        Type *type = read_type(fc, alc, scope, true);
        v = vgen_cast(alc, v, type);

        tkn = tok(fc, true, true, true);
    }

    if (prio == 0 || prio > 10) {
        while (fc->chunk_parse->token == tok_op1) {
            char sign = tkn[0];
            int op;
            if(sign == '*')
                op = op_mul;
            else if(sign == '/')
                op = op_div;
            else if(sign == '%')
                op = op_mod;
            else
                break;
            Value *right = read_value(alc, fc, scope, true, 10);
            v = value_handle_op(alc, fc, scope, v, right, op);
            tkn = tok(fc, true, true, true);
        }
    }

    if (prio == 0 || prio > 20) {
        while (fc->chunk_parse->token == tok_op1) {
            char sign = tkn[0];
            int op;
            if(sign == '+')
                op = op_add;
            else if(sign == '-')
                op = op_sub;
            else
                break;
            Value *right = read_value(alc, fc, scope, true, 20);
            v = value_handle_op(alc, fc, scope, v, right, op);
            tkn = tok(fc, true, true, true);
        }
    }

    if (prio == 0 || prio > 30) {
        while (fc->chunk_parse->token == tok_op1 || fc->chunk_parse->token == tok_op2) {
            char ch1 = tkn[0];
            char ch2 = tkn[1];
            int op;
            if(ch1 == '=' && ch2 == '=')
                op = op_eq;
            else if(ch1 == '!' && ch2 == '=')
                op = op_ne;
            else if(ch1 == '<' && ch2 == '=')
                op = op_lte;
            else if(ch1 == '>' && ch2 == '=')
                op = op_gte;
            else if(ch1 == '<' && ch2 == 0)
                op = op_lt;
            else if(ch1 == '>' && ch2 == 0)
                op = op_gt;
            else
                break;
            Value *right = read_value(alc, fc, scope, true, 30);
            v = value_handle_compare(alc, fc, scope, v, right, op);
            tkn = tok(fc, true, true, true);
        }
    }

    tok_back(fc);
    return v;
}

Value* value_handle_idf(Allocator *alc, Fc *fc, Scope *scope, Idf *idf) {
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    int type = idf->type;

    if (type == idf_decl) {
        Decl* decl = idf->item;
        return value_make(alc, v_decl, decl, decl->type);
    }
    if (type == idf_scope) {
        Scope* sub = idf->item;
        tok_expect(fc, ".", false, false);
        char *tkn = tok(fc, false, false, true);
        Idf *idf_sub = read_idf(fc, sub, tkn, true);
        return value_handle_idf(alc, fc, scope, idf_sub);
    }
    if (type == idf_func) {
        Func* func = idf->item;
        return vgen_func_ptr(alc, func, NULL);
    }
    if (type == idf_class) {
        Class* class = idf->item;
        return value_handle_class(alc, fc, scope, class);
    }

    sprintf(b->char_buf, "This identifier cannot be used inside a function. (identifier-type:%d)", idf->type);
    parse_err(chunk, b->char_buf);
    return NULL;
}

Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on) {
    Type* ont = on->rett;
    if (ont->type != type_func) {
        parse_err(fc->chunk_parse, "Function call on non-function type");
    }
    
    Build* b = fc->b;
    Array* func_args = ont->func_args;
    Type *rett = ont->func_rett;

    if (!func_args || !rett) {
        sprintf(b->char_buf, "Function pointer value is missing function type information (compiler bug)\n");
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Array* args = array_make(alc, func_args->length + 1);
    int arg_i = 0;
    if(on->type == v_func_ptr) {
        VFuncPtr* fptr = on->item;
        if(fptr->first_arg) {
            arg_i++;
            array_push(args, fptr->first_arg);
        }
    }
    int offset = arg_i;

    tok_skip_whitespace(fc);
    if (tok_id_next(fc) == tok_scope_close) {
        char *tkn = tok(fc, true, true, true);
    } else {
        // Read argument values
        while (true) {
            Value *arg = read_value(alc, fc, scope, true, 0);
            FuncArg *func_arg = array_get_index(func_args, arg_i++);
            if (func_arg) {
                type_check(fc->chunk_parse, func_arg->type, arg->rett);
            }
            array_push(args, arg);

            char *tkn = tok(fc, true, true, true);
            if (str_is(tkn, ","))
                continue;
            if (str_is(tkn, ")"))
                break;
            sprintf(b->char_buf, "Unexpected token in function arguments: '%s'\n", tkn);
            parse_err(fc->chunk_parse, b->char_buf);
        }
    }

    if(args->length > func_args->length) {
        sprintf(b->char_buf, "Too many arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
        parse_err(fc->chunk_parse, b->char_buf);
    }
    if(args->length < func_args->length) {
        sprintf(b->char_buf, "Missing arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
        parse_err(fc->chunk_parse, b->char_buf);
    }

    return vgen_func_call(alc, on, args);
}

Value* value_handle_class(Allocator *alc, Fc* fc, Scope* scope, Class* class) {
    Build* b = fc->b;
    Chunk* ch = fc->chunk_parse;
    if(tok_read_byte(fc, 0) == tok_char && tok_read_byte(fc, 1) == '.') {
        // Static functions
        char *tkn = tok(fc, true, true, true);
        die("TODO: class static functions");
        return NULL;
    }
    // Class init
    tok_expect(fc, "{", true, true);
    int propc = class->props->values->length;
    Map* values = map_make(alc);
    // Read values
    char* name = tok(fc, true, true, true);
    while(!str_is(name, "}")) {
        ClassProp* prop = map_get(class->props, name);
        if(!prop) {
            sprintf(b->char_buf, "Class '%s' has no property named: '%s'\n", class->name, name);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        if(map_contains(values, name)) {
            sprintf(b->char_buf, "Setting same property twice: '%s'\n", name);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        tok_expect(fc, ":", true, false);
        Value* val = read_value(alc, fc, scope, true, 0);
        type_check(fc->chunk_parse, prop->type, val->rett);
        map_set(values, name, val);
        name = tok(fc, true, true, true);
    }
    // Default values
    Array* props = class->props->values;
    for(int i = 0; i < propc; i++) {
        ClassProp* prop = array_get_index(props, i);
        char* name = array_get_index(class->props->keys, i);
        if(!map_contains(values, name)) {
            if(!prop->chunk_value) {
                sprintf(b->char_buf, "Missing property value for: '%s'\n", name);
                parse_err(fc->chunk_parse, b->char_buf);
            }
            Chunk backup;
            backup = *fc->chunk_parse;
            *fc->chunk_parse = *prop->chunk_value;
            Value* val = read_value(alc, fc, prop->chunk_value->fc->scope, true, 0);
            *fc->chunk_parse = backup;
            map_set(values, name, val);
        }
    }

    return value_make(alc, v_class_init, values, type_gen_class(alc, class));
}

Value* value_handle_ptrv(Allocator *alc, Fc* fc, Scope* scope) {
    tok_expect(fc, "(", false, false);
    // On
    Value *on = read_value(alc, fc, scope, true, 0);
    if (on->rett->type != type_ptr) {
        parse_err(fc->chunk_parse, "First argument of '@ptrv' must be a value of type 'ptr'");
    }
    // Type
    tok_expect(fc, ",", true, true);
    Type *type = read_type(fc, alc, scope, true);
    if (type->type == type_void) {
        parse_err(fc->chunk_parse, "You cannot use 'void' type in @ptrv");
    }
    // Index
    char* tkn = tok(fc, true, true, true);
    Value *index = NULL;
    if(str_is(tkn, ",")) {
        index = read_value(alc, fc, scope, true, 0);
        if (index->rett->type != type_int) {
            parse_err(fc->chunk_parse, "@ptrv index must be of type integer");
        }
    } else {
        tok_back(fc);
    }
    tok_expect(fc, ")", true, true);

    return vgen_ptrv(alc, fc->b, on, type, index);
}

Value* value_handle_op(Allocator *alc, Fc *fc, Scope *scope, Value *left, Value* right, int op) {
    // Check type
    Build* b = fc->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    if(!lt->class || !lt->class->allow_math) {
        parse_err(fc->chunk_parse, "You cannot use operators on these values");
    }
    bool is_ptr = false;
    bool is_signed = lt->is_signed || rt->is_signed;
    if(lt->type == type_ptr) {
        is_ptr = true;
        left = vgen_cast(alc, left, type_gen_volt(alc, b, is_signed ? "int" : "uint"));
    }
    if(rt->type == type_ptr) {
        is_ptr = true;
        right = vgen_cast(alc, right, type_gen_volt(alc, b, is_signed ? "int" : "uint"));
    }
    if(left->type == v_int && right->type != v_int) {
        left->rett = right->rett;
    } else if(right->type == v_int && left->type != v_int) {
        right->rett = left->rett;
    }

    // Try match types
    match_value_types(alc, fc->b, &left, &right);

    Type* t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        sprintf(b->char_buf, "Operator values are not compatible: %s %s %s", type_to_str(lt, t1b), op_to_str(op), type_to_str(rt, t2b));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    Value* v = vgen_op(alc, op, left, right, t1);

    if(is_ptr) {
        v = vgen_cast(alc, v, type_gen_volt(alc, b, "ptr"));
    }

    return v;
}

Value* value_handle_compare(Allocator *alc, Fc *fc, Scope *scope, Value *left, Value* right, int op) {
    Build* b = fc->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    bool is_ptr = lt->is_pointer || rt->is_pointer;
    if(is_ptr) {
        // Pointers
        if(!lt->is_pointer) {
            left = vgen_cast(alc, left, type_gen_volt(alc, b, "ptr"));
        }
        if(!rt->is_pointer) {
            right = vgen_cast(alc, right, type_gen_volt(alc, b, "ptr"));
        }
    } else {
        // Numbers
        match_value_types(alc, fc->b, &left, &right);
    }

    Type *t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        sprintf(b->char_buf, "Operator values are not compatible: %s <-> %s", type_to_str(lt, t1b), type_to_str(rt, t2b));
        parse_err(fc->chunk_parse, b->char_buf);
    }

    return vgen_comp(alc, op, left, right, type_gen_volt(alc, b, "bool"));
}

bool value_is_assignable(Value *v) {
    return v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global;
}

void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_) {
    //
    Value* v1 = *v1_;
    Value* v2 = *v2_;
    Type* t1 = v1->rett;
    Type* t2 = v2->rett;
    bool is_signed = t1->is_signed || t2->is_signed;
    bool is_float = t1->type == type_float || t2->type == type_float;
    int size = is_float ? max_num(t1->size, t2->size) : (max_num(t1->is_signed != is_signed ? t1->size * 2 : t1->size, t2->is_signed != is_signed ? t2->size * 2 : t2->size));
    Type* type = type_gen_number(alc, b, size, is_float, is_signed);
    if(!type)
        return;
    if(t1->type != type->type || t1->is_signed != type->is_signed || t1->size != type->size) {
        *v1_ = vgen_cast(alc, v1, type);
    }
    if(t2->type != type->type || t2->is_signed != type->is_signed || t2->size != type->size) {
        *v2_ = vgen_cast(alc, v2, type);
    }
}

void value_is_mutable(Value* v) {
    if (v->type == v_decl) {
        Decl *decl = v->item;
        decl->is_mut = true;
    }
}