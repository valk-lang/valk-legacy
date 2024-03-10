
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    t->size = 0;
    t->class = NULL;
    t->func_args = NULL;
    t->func_default_values = NULL;
    t->func_errors = NULL;
    t->func_rett = NULL;
    t->is_pointer = false;
    t->is_signed = false;
    t->nullable = false;
    t->func_can_error = false;
    return t;
}

Type* read_type(Parser* p, Allocator* alc, Scope* scope, bool allow_newline) {
    //
    Build* b = p->b;

    Type *type = NULL;
    bool nullable = false;
    bool is_inline = false;

    int t = tok(p, true, allow_newline, true);

    if(t == tok_at_word) {
        if (str_is(p->tkn, "@ignu")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, alc, scope, false);
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
        if (!is_inline && str_is(tkn, "void")) {
            return type_make(alc, type_void);
        }
        if (str_is(tkn, "fn")) {
            tok_expect(p, "(", false, false);
            // Args
            Array* args = array_make(alc, 4);
            while (tok(p, true, false, false) != tok_bracket_close) {
                Type* type = read_type(p, alc, scope, false);
                array_push(args, type);
                if (tok(p, true, false, false) != tok_bracket_close) {
                    tok_expect(p, ",", true, false);
                }
            }
            tok_expect(p, ")", false, false);
            tok_expect(p, "(", false, false);
            Type* rett;
            if (tok(p, true, false, false) != tok_bracket_close) {
                rett = read_type(p, alc, scope, false);
            } else {
                rett = type_gen_void(alc);
            }
            tok_expect(p, ")", false, false);
            //
            Type *t = type_make(alc, type_func);
            t->func_rett = rett;
            t->func_args = args;
            t->size = b->ptr_size;
            t->is_pointer = true;
            t->nullable = nullable;
            return t;
        }

        if (str_is(tkn, "inline")) {
            is_inline = true;
            t = tok(p, true, false, true);
        }

        if (t == tok_id) {
            // Identifier
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
                    Array *names = class->generic_names;
                    Map *generic_types = map_make(alc);
                    for (int i = 0; i < names->length; i++) {
                        char *name = array_get_index(names, i);
                        Type *type = read_type(p, alc, scope, false);
                        map_set(generic_types, name, type);
                        if (i + 1 < names->length) {
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
    return t;
}

Type* type_gen_void(Allocator* alc) {
    return type_make(alc, type_void);
}
Type* type_gen_null(Allocator* alc, Build* b) {
    Type* t = type_make(alc, type_null);
    t->is_pointer = true;
    t->size = b->ptr_size;
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
    Type* t = type_make(alc, type_func);
    t->func_rett = func->rett;
    t->func_args = func->arg_types;
    t->func_default_values = func->arg_values;
    t->func_errors = func->errors;
    t->size = func->b->ptr_size;
    t->is_pointer = true;
    t->func_can_error = func->errors ? true : false;
    return t;
}
Type* type_gen_volt(Allocator* alc, Build* b, char* name) {
    Nsc* nsc = get_volt_nsc(b, "type");
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(idf && idf->type == idf_class) {
        return type_gen_class(alc, idf->item);
    }
    printf("VOLT TYPE NOT FOUND: '%s'", name);
    exit(1);
}

Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed) {
    if(is_float) {
        if(size == 4)
            return type_gen_volt(alc, b, "f32");
        if(size == 8)
            return type_gen_volt(alc, b, "f64");
    } else {
        if(size == 1)
            return type_gen_volt(alc, b, is_signed ? "i8" : "u8");
        if(size == 2)
            return type_gen_volt(alc, b, is_signed ? "i16" : "u16");
        if(size == 4)
            return type_gen_volt(alc, b, size == b->ptr_size ? (is_signed ? "int" : "uint") : (is_signed ? "i32" : "u32"));
        if(size == 8)
            return type_gen_volt(alc, b, size == b->ptr_size ? (is_signed ? "int" : "uint") : (is_signed ? "i64" : "u64"));
    }
    return NULL;
}

bool type_compat(Type* t1, Type* t2, char** reason) {
    if (t2->type == type_null && (t1->type == type_ptr || t1->nullable || t1->ignore_null)) {
        return true;
    }
    if (t1->type != t2->type) {
        *reason = "different kind of types";
        return false;
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

bool type_is_void(Type* type) { return type->type == type_void; }
bool type_is_bool(Type* type) { return type->type == type_bool; }
bool type_is_gc(Type* type) { return type->is_pointer && type->type == type_struct && type->class->type == ct_class; }

char* type_to_str(Type* t, char* res) {

    strcpy(res, "");
    if (t->nullable) {
        strcat(res, "?");
    }
    if (t->type == type_void) {
        strcpy(res, "void");
    } else if (t->type == type_null) {
        strcpy(res, "null");
    } else if (t->class) {
        Class *class = t->class;
        strcat(res, class->name);
    }
    return res;
}

char* type_to_str_export(Type* t, char* res) {

    strcpy(res, "");
    if (t->nullable) {
        strcat(res, "NULL_");
    }
    if (t->type == type_void) {
        strcpy(res, "void");
    } else if (t->type == type_null) {
        strcpy(res, "null");
    } else if (t->class) {
        Class *class = t->class;
        strcat(res, class->ir_name);
    }
    return res;
}

void type_to_str_append(Type* t, Str* buf) {
    if (t->nullable) {
        str_flat(buf, "?");
    }
    if (t->type == type_void) {
        str_flat(buf, "void");
    } else if (t->type == type_null) {
        str_flat(buf, "null");
    } else if (t->class) {
        str_add(buf, t->class->ir_name);
    }
}

int type_get_size(Build* b, Type* type) {
    if (type->size > 0) {
        return type->size;
    }
    if (type->is_pointer) {
        return b->ptr_size;
    } else if (type->class) {
        return type->class->size;
    }
    return -1;
}
