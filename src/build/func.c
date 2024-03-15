
#include "../all.h"

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = u->b;
    f->unit = u;
    f->scope = scope_make(alc, sc_func, parent);
    f->scope_stack_reduce = NULL;
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;
    f->body_end = NULL;
    f->args = map_make(alc);
    f->arg_types = array_make(alc, 4);
    f->arg_values = array_make(alc, 4);
    f->class = NULL;
    f->cached_values = NULL;
    f->errors = NULL;
    f->is_inline = false;
    f->is_static = false;
    f->can_error = false;
    f->types_parsed = false;
    f->in_header = false;

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

void parse_handle_func_args(Parser* p, Func* func) {
    Build* b = p->b;

    tok_expect(p, "(", true, false);
    func->chunk_args = chunk_clone(b->alc, p->chunk);
    skip_body(p);

    if(func->in_header) {
        func->chunk_rett = chunk_clone(b->alc, p->chunk);
        skip_type(p);
    } else {
        char t = tok(p, true, true, false);
        if (t != tok_curly_open && t != tok_not) {
            // Has return type
            func->chunk_rett = chunk_clone(b->alc, p->chunk);
            skip_type(p);
        }
    }

    char t = tok(p, true, true, false);
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
        FuncError* err = NULL;
        if(func->in_header) {
            build_err(b, "TODO: header errors");
        } else {
            err = map_get(b->errors->errors, name);
            if(!err) {
                err = al(b->alc, sizeof(FuncError));
                err->value = ++b->error_count;
                err->collection = b->errors;
                map_set(b->errors->errors, name, err);
            }
        }
        Idf* idf = idf_make(b->alc, idf_error, err);
        map_set(errors, name, err);
        scope_set_idf(func->scope, name, idf, p);

        t = tok(p, true, true, false);
    }
    func->errors = errors;

    if(!func->in_header) {
        tok_expect(p, "{", true, true);

        Chunk *chunk_end = chunk_clone(b->alc, p->chunk);
        chunk_end->i = p->scope_end_i;
        func->body_end = chunk_end;

        func->chunk_body = chunk_clone(b->alc, p->chunk);
        skip_body(p);
    }
}

char* ir_func_err_handler(IR* ir, Scope* scope, char* res, VFuncCall* fcall) {
    if(!fcall->err_scope && !fcall->err_value) {
        return res;
    }

    IRBlock *block_err = ir_block_make(ir, ir->func, "if_err_");
    IRBlock *block_else = fcall->err_value ? ir_block_make(ir, ir->func, "if_not_err_") : NULL;
    IRBlock *after = ir_block_make(ir, ir->func, "if_err_after_");

    Type* type_i32 = type_gen_volt(ir->alc, ir->b, "i32");
    char *load = ir_load(ir, type_i32, "@volt_err_code");
    char *lcond = ir_compare(ir, op_ne, load, "0", "i32", false, false);

    // Clear error
    ir_store_old(ir, type_i32, "@volt_err_code", "0");

    ir_cond_jump(ir, lcond, block_err, fcall->err_value ? block_else : after);

    if(fcall->err_scope) {

        Scope* err_scope = fcall->err_scope;
        ir->block = block_err;
        ir_write_ast(ir, err_scope);
        ir->block = after;

        return res;

    } else if(fcall->err_value) {

        Value* val = fcall->err_value;
        char* ltype = ir_type(ir, val->rett);

        ir->block = block_err;
        char* alt_val = ir_value(ir, scope, val);
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
        str_add(code, res);
        str_flat(code, ", %");
        str_add(code, block_else->name);
        str_flat(code, " ], [ ");
        str_add(code, alt_val);
        str_flat(code, ", %");
        str_add(code, block_err->name);
        str_flat(code, " ]\n");

        return var;
    }
}


void func_generate_args(Allocator* alc, Func* func, Map* args) {
    int count = args->values->length;
    for(int i = 0; i < count; i++) {
        char* name = array_get_index(args->keys, i);
        Type* type = array_get_index(args->values, i);

        FuncArg *arg = func_arg_make(alc, type);
        map_set_force_new(func->args, name, arg);
        array_push(func->arg_types, arg->type);
        Decl *decl = decl_make(alc, name, arg->type, true);
        Idf *idf = idf_make(alc, idf_decl, decl);
        scope_set_idf(func->scope, name, idf, NULL);
        arg->decl = decl;
        array_push(func->arg_values, NULL);
    }
}
