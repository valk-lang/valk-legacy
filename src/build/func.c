
#include "../all.h"

void func_check_error_dupe(Parser* p, Func* func, Map* errors, char* str_v, unsigned int v);

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = u->b;
    f->unit = u;
    f->act = act_public;
    f->fc = NULL;
    f->scope = scope_make(alc, sc_func, parent);
    f->scope->func = f;
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;
    f->reference_type = NULL;

    f->args = map_make(alc);
    f->arg_types = array_make(alc, 4);
    f->arg_values = array_make(alc, 4);
    f->rett_types = array_make(alc, 1);
    f->used_functions = array_make(alc, 4);
    f->used_classes = array_make(alc, 4);

    f->class = NULL;
    f->cached_values = NULL;
    f->errors = NULL;

    f->ast_stack_init = NULL;
    f->ast_start = NULL;
    f->ast_end = NULL;
    // Cached values
    f->v_cache_stack = NULL;
    f->v_cache_stack_pos = NULL;

    f->alloca_size = 0;
    f->decl_nr = 0;
    f->arg_nr = 0;


    f->is_inline = false;
    f->is_static = false;
    f->can_error = false;
    f->types_parsed = false;
    f->in_header = false;
    f->has_rett = false;
    f->multi_rett = false;
    f->is_test = false;
    f->is_used = false;
    f->use_if_class_is_used = false;
    f->exits = false;
    f->parsed = false;
    f->calls_gc_check = false;
    f->parse_last = false;
    f->init_thread = false;

    if (!export_name)
        f->export_name = gen_export_name(u->nsc, name);

    array_push(u->funcs, f);

    return f;
}

FuncArg* func_arg_make(Allocator* alc, Type* type) {
    FuncArg* a = al(alc, sizeof(FuncArg));
    a->type = type;
    a->value = NULL;
    a->chunk_value = NULL;
    return a;
}

void func_mark_used(Func* func, Func* uses_func) {
    if(!func)
        return;
    array_push(func->used_functions, uses_func);
}

void parse_handle_func_args(Parser* p, Func* func) {
    Build* b = p->b;

    tok_expect(p, "(", true, false);
    func->chunk_args = chunk_clone(b->alc, p->chunk);
    skip_body(p);

    // Return type
    func->chunk_rett = chunk_clone(b->alc, p->chunk);

    char t = tok(p, true, true, false);
    if (t != tok_curly_open && t != tok_not) {
        func->has_rett = true;
        if(t == tok_bracket_open) {
            func->multi_rett = true;
            skip_body(p);
        } else {
            skip_type(p);
        }
    }

    t = tok(p, true, true, false);
    Map *errors = NULL;
    if (t == tok_not) {
        errors = map_make(b->alc);
    }
    while(t == tok_not) {
        tok(p, true, true, true);
        t = tok(p, false, false, true);
        if(t != tok_id) {
            parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
        }
        // Get error value
        char* name = p->tkn;
        unsigned int err = 0;
        if(func->in_header) {
            build_err(b, "TODO: header errors");
        } else {
            unsigned int v = ctxhash_u32(name);
            if(v == 0)
                v = 1;
            func_check_error_dupe(p, func, errors, name, v);
            err = v;
        }
        map_set(errors, name, (void*)(uintptr_t)err);

        t = tok(p, true, true, false);
    }
    func->errors = errors;

    if(!func->in_header) {
        tok_expect(p, "{", true, true);
        func->chunk_body = chunk_clone(b->alc, p->chunk);
        skip_body(p);
    } else {
        tok_expect(p, ";", true, true);
    }
}

char* ir_func_err_handler(IR* ir, ErrorHandler* errh, char* on, bool on_await){

    IRBlock *block_err = ir_block_make(ir, ir->func, "if_err_");
    IRBlock *block_else = errh->err_value ? ir_block_make(ir, ir->func, "if_not_err_") : NULL;
    IRBlock *after = ir_block_make(ir, ir->func, "if_err_after_");

    Type *type_i32 = type_gen_valk(ir->alc, ir->b, "i32");

    char *coro_err_prop = NULL;
    char *load = NULL;
    if (on_await) {
        Class *coro_class = get_valk_class(ir->b, "core", "Coro2");
        coro_err_prop = ir_class_pa(ir, coro_class, on, map_get(coro_class->props, "error"));
        load = ir_load(ir, type_i32, coro_err_prop);
    } else {
        load = ir_load(ir, type_i32, "@valk_err_code");
    }

    char *lcond = ir_compare(ir, op_ne, load, "0", "i32", false, false);

    if (errh->err_decl) {
        if(errh->err_decl->is_mut) {
            ir_store(ir, ir_decl_var(ir, errh->err_decl), load, "i32", type_i32->size);
        } else {
            errh->err_decl->custom_ir_name = load;
            // ir_decl_set(ir, errh->err_decl, load);
        }
    }

    ir_cond_jump(ir, lcond, block_err, errh->err_value ? block_else : after);

    if(errh->err_scope) {

        Scope* err_scope = errh->err_scope;
        ir->block = block_err;
        if(on_await) {
            ir_store_old(ir, type_i32, coro_err_prop, "0");
        } else {
            ir_store_old(ir, type_i32, "@valk_err_code", "0");
        }

        ir_write_ast(ir, err_scope);
        if(!err_scope->did_return) {
            ir_jump(ir, after);
        }
        ir->block = after;

        return on;

    } else if(errh->err_value) {

        Value* val = errh->err_value;
        char* ltype = ir_type(ir, val->rett);

        ir->block = block_err;
        if(on_await) {
            ir_store_old(ir, type_i32, coro_err_prop, "0");
        } else {
            ir_store_old(ir, type_i32, "@valk_err_code", "0");
        }

        char* alt_val = ir_value(ir, val);
        IRBlock* block_err_val = ir->block;
        ir_jump(ir, after);

        ir->block = block_else;
        ir_jump(ir, after);

        ir->block = after;
        Str* code = after->code;
        char* var = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, " = phi ");
        str_add(code, ltype);
        str_flat(code, " [ ");
        str_add(code, on);
        str_flat(code, ", %");
        str_add(code, block_else->name);
        str_flat(code, " ], [ ");
        str_add(code, alt_val);
        str_flat(code, ", %");
        str_add(code, block_err_val->name);
        str_flat(code, " ]\n");

        return var;
    }

    return NULL;
}

void func_validate_arg_count(Parser* p, Func* func, bool is_static, int arg_count_min, int arg_count_max) {

    if (func->is_static != is_static) {
        *p->chunk = *func->chunk_args;
        if (is_static)
            parse_err(p, -1, "Expected function to be static");
        else
            parse_err(p, -1, "Expected function to be non-static");
    }

    int argc = func->arg_types->length;
    int offset = !func->class || is_static ? 0 : 1;
    if (argc < arg_count_min) {
        *p->chunk = *func->chunk_args;
        parse_err(p, -1, "Expected amount of function arguments: %d, instead of: %d", arg_count_min - offset, argc - offset);
    }
    if (argc > arg_count_max) {
        *p->chunk = *func->chunk_args;
        parse_err(p, -1, "Expected amount of function arguments: %d, instead of: %d", arg_count_max - offset, argc - offset);
    }
}
void func_validate_rett_count(Parser* p, Func* func, bool is_static, int rett_count_min, int rett_count_max) {

    if (func->is_static != is_static) {
        *p->chunk = *func->chunk_args;
        if (is_static)
            parse_err(p, -1, "Expected function to be static");
        else
            parse_err(p, -1, "Expected function to be non-static");
    }

    int rettc = func->rett_types->length;
    if (rettc < rett_count_min) {
        *p->chunk = *func->chunk_rett;
        parse_err(p, -1, "Expected amount of function return-types: %d, instead of: %d", rett_count_min, rettc);
    }
    if (rettc > rett_count_max) {
        *p->chunk = *func->chunk_rett;
        parse_err(p, -1, "Expected amount of function return-types: %d, instead of: %d", rett_count_max, rettc);
    }
}
void func_validate_arg_type(Parser* p, Func* func, int index, Array* allowed_types) {
    bool has_valid = false;
    FuncArg* arg = array_get_index(func->args->values, index);
    Type *rett = arg->type;
    int count = allowed_types->length;
    for (int i = 0; i < count; i++) {
        Type *valid = array_get_index(allowed_types, i);
        if(rett->class == valid->class && rett->nullable == valid->nullable && rett->is_pointer == valid->is_pointer) {
            has_valid = true;
            break;
        }
    }
    if(!has_valid){
        *p->chunk = *arg->chunk;
        char types[2048];
        char buf[512];
        types[0] = 0;
        for (int i = 0; i < count; i++) {
            Type *valid = array_get_index(allowed_types, i);
            type_to_str(valid, buf);
            if(i > 0) {
                strcat(types, ", ");
            }
            strcat(types, buf);
        }
        type_to_str(rett, buf);
        parse_err(p, -1, "Function return type is '%s', but should be on of the following: %s", buf, types);
    }
}
void func_validate_rett(Parser* p, Func* func, Array* allowed_types) {
    bool has_valid = false;
    Type *rett = func->rett;
    int count = allowed_types->length;
    for (int i = 0; i < count; i++) {
        Type *valid = array_get_index(allowed_types, i);
        if(rett->class == valid->class && rett->nullable == valid->nullable && rett->is_pointer == valid->is_pointer) {
            has_valid = true;
            break;
        }
    }
    if(!has_valid){
        *p->chunk = *func->chunk_rett;
        char types[2048];
        char buf[512];
        types[0] = 0;
        for (int i = 0; i < count; i++) {
            Type *valid = array_get_index(allowed_types, i);
            type_to_str(valid, buf);
            if(i > 0) {
                strcat(types, ", ");
            }
            strcat(types, buf);
        }
        type_to_str(rett, buf);
        parse_err(p, -1, "Function return type is '%s', but should be on of the following: %s", buf, types);
    }
}

void func_validate_rett_void(Parser *p, Func *func) {
    if (!type_is_void(func->rett)) {
        *p->chunk = *func->chunk_rett;
        char buf[512];
        type_to_str(func->rett, buf);
        parse_err(p, -1, "Expected function return type to be 'void' instead of '%s'", buf);
    }
}

void func_check_error_dupe(Parser* p, Func* func, Map* errors, char* str_v, unsigned int v) {
    if(map_get(errors, str_v)) {
        parse_err(p, -1, "Duplicate error name");
    }
    int len = errors->values->length;
    for(int i = 0; i < len; i++) {
        unsigned int ev = array_get_index_u32(errors->values, i);
        if(ev == v) {
            char* prev = array_get_index(errors->keys, i);
            parse_err(p, -1, "Error '%s' and '%s' have the same error hash value, you must rename one of them. The error value is based on the hash value of the error name. There is no other solution than renaming one of them.", prev, str_v);
        }
    }
}
