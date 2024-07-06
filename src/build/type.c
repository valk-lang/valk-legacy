
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    t->size = 0;
    t->array_size = 0;
    t->class = NULL;
    t->func_info = NULL;
    t->array_type = NULL;
    t->multi_types = NULL;
    t->is_pointer = false;
    t->is_signed = false;
    t->nullable = false;
    t->ignore_null = false;
    return t;
}
TypeFuncInfo* type_func_info_make(Allocator* alc, Array* args, bool inf_args, Array* default_values, Array* err_names, Array* err_values, Type* rett) {
    TypeFuncInfo* t = al(alc, sizeof(TypeFuncInfo));
    t->args = args;
    t->default_values = default_values;
    t->err_names = err_names;
    t->err_values = err_values;
    t->rett = rett;
    t->has_unknown_errors = false;
    t->can_error = false;
    t->will_exit = false;
    t->inf_args = inf_args;
    return t;
}

Type* read_type(Parser* p, Allocator* alc, bool allow_newline) {
    //
    Build* b = p->b;
    Scope* scope = p->scope;

    Type *type = NULL;
    bool nullable = false;
    bool is_inline = false;
    bool allow_multi = false;
    if(p->allow_multi_type) {
        allow_multi = true;
        p->allow_multi_type = false;
    }

    char t = tok(p, true, allow_newline, true);

    if(t == tok_bracket_open && allow_multi) {
        // Void
        t = tok(p, true, allow_newline, false);
        if(t == tok_bracket_close) {
            t = tok(p, true, allow_newline, true);
            return type_gen_void(alc);
        }
        // Multi types
        Array* types = array_make(alc, 2);
        while (true) {
            Type *type = read_type(p, b->alc, false);
            if (type_is_void(type)) {
                parse_err(p, -1, "You cannot use 'void' in a type group");
            }
            array_push(types, type);
            t = tok_expect_two(p, ",", ")", true, true);
            if (t == tok_bracket_close)
                break;
        }
        if(types->length == 1) {
            // Return single type if only 1 type
            return array_get_index(types, 0);
        }
        Type* mt = type_make(alc, type_multi);
        mt->multi_types = types;
        return mt;
    }

    if(t == tok_at_word) {
        if (str_is(p->tkn, "@ignu")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, alc, false);
            tok_expect(p, ")", true, false);
            type->nullable = false;
            type->ignore_null = true;
            return type;
        }
    }

    if(t == tok_qmark) {
        nullable = true;
        t = tok(p, false, false, true);
    }

    if(t == tok_id) {
        char* tkn = p->tkn;
        if (str_is(tkn, "void")) {
            return type_make(alc, type_void);
        }
        if (str_is(tkn, "fn")) {
            tok_expect(p, "(", false, false);
            // Args
            Array* args = array_make(alc, 4);
            while (tok(p, true, false, false) != tok_bracket_close) {
                Type* type = read_type(p, alc, false);
                array_push(args, type);
                if (tok(p, true, false, false) != tok_bracket_close) {
                    tok_expect(p, ",", true, false);
                }
            }
            tok_expect(p, ")", false, false);
            // Return types
            int before_i = p->chunk->i;
            tok_expect(p, "(", false, false);
            p->chunk->i = before_i;
            p->allow_multi_type = true;
            Type* rett = read_type(p, alc, false);
            // Errors
            // TODO

            // Generate type
            Type *t = type_make(alc, type_func);
            t->func_info = type_func_info_make(alc, args, false, NULL, NULL, NULL, rett);
            t->size = b->ptr_size;
            t->is_pointer = true;
            t->nullable = nullable;
            return t;
        }

        if (str_is(tkn, "inline")) {
            is_inline = true;
            t = tok(p, true, false, true);
            tkn = p->tkn;
        }
    }

    if (t == tok_sq_bracket_open) {
        Type *array_type = read_type(p, alc, false);
        tok_expect(p, ",", true, false);
        t = tok(p, true, false, true);
        if (t != tok_number) {
            parse_err(p, -1, "Expected a valid number here to define the length of the static array");
        }
        int itemc = atoi(p->tkn);
        if (itemc == 0) {
            parse_err(p, -1, "Static array size must be larger than 0, size: '%s'", p->tkn);
        }
        tok_expect(p, "]", true, false);
        Type *t = type_make(alc, type_static_array);
        t->array_type = array_type;
        t->is_pointer = !is_inline;
        t->size = is_inline ? (array_type->size * itemc) : b->ptr_size;
        t->array_size = itemc;
        return t;
    }

    if (t == tok_id) {
        // Identifier
        char* tkn = p->tkn;
        Id id;
        read_id(p, tkn, &id);
        Idf *idf = idf_by_id(p, scope, &id, false);
        if (!idf) {
            id.ns ? parse_err(p, -1, "Unknown type: '%s:%s'", id.ns, id.name)
                  : parse_err(p, -1, "Unknown type: '%s'", id.name);
        }
        if (idf->type == idf_class) {
            Class *class = idf->item;

            if (class->is_generic_base) {
                tok_expect(p, "[", false, false);
                int count = class->generic_names->length;
                Array *generic_types = array_make(alc, count + 1);
                for (int i = 0; i < count; i++) {
                    Type *type = read_type(p, alc, false);
                    array_push(generic_types, type);
                    if (i + 1 < count) {
                        tok_expect(p, ",", true, false);
                    } else {
                        tok_expect(p, "]", true, false);
                        break;
                    }
                }
                class = get_generic_class(p, class, generic_types);
            }

            type = type_gen_class(alc, class);
        }
        if (idf->type == idf_type) {
            type = type_clone(alc, idf->item);
        }
    }

    if (type) {
        if (is_inline) {
            type->is_pointer = false;
            if(type->type == type_struct) {
                type->size = type->class->size;
            }
        }
        if (nullable) {
            if (!type->is_pointer) {
                char buf[256];
                parse_err(p, -1, "This type cannot be null: '?%s'", type_to_str(type, buf));
            }
            type->nullable = true;
        }
        return type;
    }

    parse_err(p, -1, "Invalid type: '%s'", p->tkn);
    return NULL;
}
Type* type_clone(Allocator* alc, Type* type) {
    Type* t = al(alc, sizeof(Type));
    *t = *type;
    t->func_info = type_clone_function_info(alc, type->func_info);
    return t;
}

TypeFuncInfo* type_clone_function_info(Allocator* alc, TypeFuncInfo* fi) {
    if(!fi)
        return NULL;

    TypeFuncInfo* fi2 = al(alc, sizeof(TypeFuncInfo));
    *fi2 = *fi;
    fi2->args = fi->args ? clone_array_of_types(alc, fi->args) : NULL;
    fi2->default_values = fi->default_values ? array_make(alc, fi->default_values->length + 1) : NULL;
    fi2->err_names = fi->err_names ? array_make(alc, fi->err_names->length + 1) : NULL;
    fi2->err_values = fi->err_values ? array_make(alc, fi->err_values->length + 1) : NULL;
    fi2->rett = fi->rett ? type_clone(alc, fi->rett) : NULL;

    return fi2;
}

Array *clone_array_of_types(Allocator *alc, Array *types) {
    Array *list = array_make(alc, types->length + 1);
    loop(types, i) {
        Type* t = array_get_index(types, i);
        array_push(list, type_clone(alc, t));
    }
    return list;
}

Type* type_gen_void(Allocator* alc) {
    return type_make(alc, type_void);
}
Type* type_gen_null(Allocator* alc, Build* b) {
    Type* t = type_make(alc, type_null);
    t->is_pointer = true;
    t->size = b->ptr_size;
    t->nullable = true;
    return t;
}
Type* type_gen_class(Allocator* alc, Class* class) {
    Type* t = type_make(alc, type_struct);
    t->class = class;
    t->size = class->b->ptr_size;

    const int ct = class->type;
    if(ct == ct_bool) {
        t->type = type_bool;
    } else if (ct == ct_ptr) {
        t->type = type_ptr;
    } else if (ct == ct_int) {
        t->type = type_int;
        t->size = class->size;
        t->is_signed = class->is_signed;
    } else if (ct == ct_float) {
        t->type = type_float;
    }

    if(ct == ct_class || ct == ct_struct || ct == ct_ptr)
        t->is_pointer = true;

    return t;
}
Type* type_gen_func(Allocator* alc, Func* func) {
    if (!func->reference_type) {
        Type *t = type_make(alc, type_func);
        t->size = func->b->ptr_size;
        t->is_pointer = true;
        t->func_info = type_func_info_make(alc, func->arg_types, func->inf_args, func->arg_values, func->errors ? func->errors->keys : NULL, func->errors ? func->errors->values : NULL, func->rett);
        t->func_info->can_error = func->errors ? true : false;
        t->func_info->will_exit = func->exits;
        func->reference_type = t;
    }
    return func->reference_type;
}

Type* type_gen_error(Allocator* alc, Array* err_names, Array* err_values) {
    Type* t = type_make(alc, type_error);
    t->func_info = type_func_info_make(alc, NULL, false, NULL, err_names, err_values, NULL);
    t->size = sizeof(int);
    return t;
}

Type* type_gen_promise(Allocator* alc, Build* b, TypeFuncInfo* fi) {
    Type* t = type_make(alc, type_promise);
    t->func_info = fi;
    t->is_pointer = true;
    t->size = b->ptr_size;
    // t->class = type_gen_class(alc, get_valk_class(, "core", "Coro"));
    return t;
}
Type* type_gen_multi(Allocator* alc, Array* types) {
    if (types->length == 1)
        return array_get_index(types, 0);
    Type *t = type_make(alc, type_multi);
    t->multi_types = types;
    return t;
}


Type* type_gen_valk(Allocator* alc, Build* b, char* name) {
    char *ns = "type";
    if (name[0] == 'u' && str_is(name, "uint")) {
        name = get_number_type_name(b, b->ptr_size, false, false);
    } else if (name[0] == 'i' && str_is(name, "int")) {
        name = get_number_type_name(b, b->ptr_size, false, true);
    } else if (name[0] == 'f' && str_is(name, "float")) {
        name = get_number_type_name(b, b->ptr_size, true, false);
    } else if (name[0] == 'F' && str_is(name, "FD")) {
        ns = "io";
    }
    Nsc* nsc = get_valk_nsc(b, ns);
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(idf && idf->type == idf_class) {
        return type_gen_class(alc, idf->item);
    }
    printf("VALK TYPE NOT FOUND: '%s'", name);
    exit(1);
}

Type *type_gen_valk_class(Allocator *alc, Build *b, char *ns, char *name, bool nullable) {
    Class* class = get_valk_class(b, ns, name);
    Type* t = type_gen_class(alc, class);
    t->nullable = nullable;
    return t;
}

char* get_number_type_name(Build* b, int size, bool is_float, bool is_signed) {
    if(is_float) {
        if(size == 4)
            return "f32";
        if(size == 8)
            return "f64";
    } else {
        if(size == 1)
            return is_signed ? "i8" : "u8";
        if(size == 2)
            return is_signed ? "i16" : "u16";
        if(size == 4)
            return is_signed ? "i32" : "u32";
        if(size == 8)
            return is_signed ? "i64" : "u64";
    }
    return NULL;
}

Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed) {
    char* name = get_number_type_name(b, size, is_float, is_signed);
    if(name) {
        return type_gen_valk(alc, b, name);
    }
    return NULL;
}

Type* type_merge(Allocator* alc, Type* t1, Type* t2) {
    if (t1->class != t2->class)
        return NULL;
    if (t1->is_pointer != t2->is_pointer)
        return NULL;
    Type* type = type_make(alc, t1->type);
    *type = *t1;
    type->nullable = t1->nullable || t2->nullable;
    return type;
}

bool type_compat(Type* t1, Type* t2, char** reason) {
    if (!t1 && type_is_void(t2))
        return true;
    if (!t2 && type_is_void(t1))
        return true;
    if (!t1 || !t2)
        return false;
    if (t2->type == type_null && (t1->type == type_ptr || t1->nullable || t1->ignore_null)) {
        return true;
    }
    if (t1->type != t2->type) {
        *reason = "different kind of types";
        return false;
    }
    if (t1->type == type_multi) {
        if(t1->multi_types->length != t2->multi_types->length)
            return false;
        loop(t1->multi_types, i) {
            Type* tt1 = array_get_index(t1->multi_types, i);
            Type* tt2 = array_get_index(t2->multi_types, i);
            if(!type_compat(tt1, tt2, reason)) {
                return false;
            }
        }
        return true;
    }
    if (t1->is_signed != t2->is_signed) {
        *reason = "signed vs unsigned";
        return false;
    }
    if (t1->size != t2->size) {
        *reason = "types have different sizes";
        return false;
    }
    if (t1->is_pointer != t2->is_pointer) {
        *reason = "pointer vs no-pointer";
        return false;
    }
    if (t1->type == type_struct && t1->class != t2->class) {
        *reason = "different classes";
        return false;
    }
    if (!t1->ignore_null && t2->nullable && !t1->nullable) {
        *reason = "non-null vs null-able-type";
        return false;
    }
    if (t1->type == type_func) {
        Array* t1s = t1->func_info->args;
        Array* t2s = t2->func_info->args;
        if(t1s->length != t2s->length){
            *reason = "different amount of argument types";
            return false;
        }
        loop(t1s, i) {
            Type *ft1 = array_get_index(t1s, i);
            Type* ft2 = array_get_index(t2s, i);
            if (!type_compat(ft2, ft1, reason)) {
                *reason = "argument types not compatible";
                return false;
            }
        }
        // Return types
        Type* t1r = t1->func_info->rett;
        Type* t2r = t2->func_info->rett;
        if (!type_compat(t1r, t2r, reason)) {
            *reason = "return types not compatible";
            return false;
        }
    }
    return true;
}
void type_check(Parser* p, Type* t1, Type* t2) {
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)) {
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Types are not compatible | %s <-> %s | Reason: %s", type_to_str(t1, t1b), type_to_str(t2, t2b), reason ? reason : "?");
    }
}

bool type_is_void(Type* type) { return type ? (type->type == type_void) : true; }
bool type_is_bool(Type* type) { return type->type == type_bool; }
bool type_is_gc(Type* type) { return type->is_pointer && type->type == type_struct && type->class->type == ct_class; }
bool type_fits_pointer(Type* type, Build* b){ return type->size <= b->ptr_size; }

bool types_contain_void(Array* types) {
    int len = types->length;
    for(int i = 0; i < len; i++){
        Type* type = array_get_index(types, i);
        if (type->type == type_void)
            return true;
    }
    return false;
}

void type_to_str_append(Type* t, char* res, bool use_export_name) {
    if (!t || t->type == type_void) {
        strcat(res, "void");
        return;
    } else if (t->type == type_null) {
        strcat(res, "null");
        return;
    }
    if (t->nullable) {
        strcat(res, use_export_name ? "NULL_" : "?");
    }
    if (t->type == type_multi) {
        // strcat(res, use_export_name ? "_" : "(");
        Array * types = t->multi_types;
        loop(types, i) {
            if(i > 0)
                strcat(res, use_export_name ? "_" : ", ");
            Type* sub = array_get_index(types, i);
            type_to_str_append(sub, res, use_export_name);
        }
        // strcat(res, use_export_name ? "_" : ")");
    }
    if (t->type == type_func) {
        strcat(res, use_export_name ? "fn_" : "fn(");
        Array * types = t->func_info->args;
        loop(types, i) {
            if(i > 0) {
                strcat(res, use_export_name ? "_" : ", ");
            }
            Type* sub = array_get_index(types, i);
            type_to_str_append(sub, res, use_export_name);
        }
        strcat(res, use_export_name ? "__" : ")(");
        Type* rett = t->func_info->rett;
        type_to_str_append(rett, res, use_export_name);
        strcat(res, use_export_name ? "_" : ")");
    } else if (t->type == type_promise) {
        strcat(res, use_export_name ? "PROMISE_" : "promise(");
        Type* rett = t->func_info->rett;
        type_to_str_append(rett, res, use_export_name);
        strcat(res, use_export_name ? "_" : ")");
    } else if (t->class) {
        Class *class = t->class;
        strcat(res, use_export_name ? class->ir_name : class->name);
    }
}
char* type_to_str(Type* t, char* res) {
    strcpy(res, "");
    type_to_str_append(t, res, false);
    return res;
}
char* type_to_str_export(Type* t, char* res) {
    strcpy(res, "");
    type_to_str_append(t, res, true);
    return res;
}

void type_to_str_buf_append(Type* t, Str* buf) {
    char* tmp = ((char*)(buf->data)) + buf->length;
    tmp[0] = 0;
    type_to_str_append(t, tmp, true);
    buf->length += strlen(tmp);
}

int type_get_size(Build* b, Type* type) {
    if (type->size > 0) {
        return type->size;
    }
    if (type->is_pointer) {
        return b->ptr_size;
    } else if (type->class) {
        return type->class->size;
    } else if (type->array_type) {
        return type_get_size(b, type->array_type) * type->array_size;
    }
    return -1;
}

Array* gen_type_array_1(Allocator* alc, Build* b, char* type1, bool nullable) {
    Array* types = array_make(alc, 2);
    Type* t1 = type_gen_valk(alc, b, type1);
    t1->nullable = nullable;
    array_push(types, t1);
    return types;
}
Array* gen_type_array_2(Allocator* alc, Build* b, char* type1, bool nullable1, char* type2, bool nullable2) {
    Array* types = array_make(alc, 2);

    Type* t1 = type1 ? type_gen_valk(alc, b, type1) : type_gen_void(alc);
    t1->nullable = nullable1;
    array_push(types, t1);

    Type* t2 = type2 ? type_gen_valk(alc, b, type2) : type_gen_void(alc);
    t2->nullable = nullable2;
    array_push(types, t2);

    return types;
}


Type* vscope_get_result_type(Array* values) {
    Value *first = array_get_index(values, 0);
    if(!first)
        return NULL;
    Type *type = first->rett;
    bool contains_nullable = false;
    loop(values, i) {
        Value *v = array_get_index(values, i);
        if (v->rett->nullable) {
            contains_nullable = true;
            break;
        }
    }
    if (type->is_pointer)
        type->nullable = contains_nullable;
    return type;
}

Type* tcache_ptr;
Type* type_cache_ptr(Build* b) {
    if(!tcache_ptr)
        tcache_ptr = type_gen_valk(b->alc, b, "ptr");
    return tcache_ptr;
}

Type* tcache_uint;
Type* type_cache_uint(Build* b) {
    if(!tcache_uint)
        tcache_uint = type_gen_valk(b->alc, b, "uint");
    return tcache_uint;
}

Type* tcache_u8;
Type* type_cache_u8(Build* b) {
    if(!tcache_u8)
        tcache_u8 = type_gen_valk(b->alc, b, "u8");
    return tcache_u8;
}
Type* tcache_u32;
Type* type_cache_u32(Build* b) {
    if(!tcache_u32)
        tcache_u32 = type_gen_valk(b->alc, b, "u32");
    return tcache_u32;
}
Type* tcache_i32;
Type* type_cache_i32(Build* b) {
    if(!tcache_i32)
        tcache_i32 = type_gen_valk(b->alc, b, "i32");
    return tcache_i32;
}

Type* class_pool_type(Parser* p, Class* class) {
    Build* b = p->b;
    Allocator* alc = b->alc;
    Type* gt = type_gen_class(alc, class);
    Array* gtypes = array_make(alc, 2);
    array_push(gtypes, gt);
    Class* base = get_valk_class(b, "mem", "GcPool2");
    Class* gen = get_generic_class(p, base, gtypes);
    Type* type = type_gen_class(alc, gen);
    type->ignore_null = true;
    return type;
}

Array* rett_types_of(Allocator* alc, Type* type) {
    if(!type)
        return NULL;
    if(type->type == type_void)
        return NULL;
    if(type->multi_types || type->type == type_multi)
        return type->multi_types;
    Array* arr = array_make(alc, 1);
    // type->multi_types = arr;
    array_push(arr, type);
    return arr;
}

Type* rett_extract_eax(Build* b, Type* type) {
    if(!type)
        return NULL;
    if(type->type == type_multi) {
        type = array_get_index(type->multi_types, 0);
    }
    if(type_fits_pointer(type, b))
        return type;
    return NULL;
}
