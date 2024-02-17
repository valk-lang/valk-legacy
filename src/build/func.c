
#include "../all.h"

Func* func_make(Allocator* alc, Fc* fc, Scope* parent, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = fc->b;
    f->fc = fc;
    f->scope = scope_make(alc, sc_func, parent);
    f->scope_gc_pop = NULL;
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;
    f->args = map_make(alc);
    f->arg_types = array_make(alc, 4);
    f->arg_values = array_make(alc, 4);
    f->class = NULL;
    f->cached_values = NULL;
    f->errors = NULL;
    f->is_inline = false;
    f->is_static = false;
    f->can_error = false;

    f->scope->func = f;

    if(!f->export_name) {
        f->export_name = gen_export_name(fc->nsc, name);
    }

    return f;
}

FuncArg* func_arg_make(Allocator* alc, Type* type) {
    FuncArg* a = al(alc, sizeof(FuncArg));
    a->type = type;
    a->value = NULL;
    a->chunk_value = NULL;
    return a;
}

void parse_handle_func_args(Fc* fc, Func* func) {
    Build* b = fc->b;

    tok_expect(fc, "(", true, false);
    func->chunk_args = chunk_clone(fc->alc, fc->chunk_parse);
    skip_body(fc);

    if(fc->is_header) {
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        skip_type(fc);
    } else {
        char *tkn = tok(fc, true, true, true);
        if (!str_is(tkn, "{") && !str_is(tkn, "!")) {
            // Has return type
            tok_back(fc);
            func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
            skip_type(fc);
        } else {
            tok_back(fc);
        }
    }

    char *tkn = tok(fc, true, true, true);
    Map *errors = NULL;
    if (str_is(tkn, "!")) {
        errors = map_make(b->alc);
    }
    while(str_is(tkn, "!")) {
        char *name = tok(fc, false, false, true);
        if(!is_valid_varname(name)) {
            sprintf(b->char_buf, "Invalid error name: '%s'", name);
            parse_err(fc->chunk_parse, b->char_buf);
        }
        // Get error value
        FuncError* err = NULL;
        if(fc->is_header) {
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
        scope_set_idf(func->scope, name, idf, fc);

        tkn = tok(fc, true, true, true);
    }
    func->errors = errors;
    tok_back(fc);

    if(!fc->is_header) {
        tok_expect(fc, "{", true, true);

        Chunk *chunk_end = chunk_clone(fc->alc, fc->chunk_parse);
        chunk_end->i = chunk_end->scope_end_i;
        func->scope->chunk_end = chunk_end;

        func->chunk_body = chunk_clone(fc->alc, fc->chunk_parse);
        skip_body(fc);
    }
}


int func_get_reserve_count(Func* func) {
    // Decls
    int count = 0;
    Scope* scope = func->scope;
    Array* decls = scope->decls;
    for (int i = 0; i < decls->length; i++) {
        Decl* decl = array_get_index(decls, i);
        if(decl->is_gc) {
            count++;
        }
    }
    return count;
}
