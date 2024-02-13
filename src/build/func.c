
#include "../all.h"

Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = fc->b;
    f->fc = fc;
    f->scope = scope_make(alc, sc_func, fc->scope);
    f->scope_gc_pop = NULL;
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;
    f->args = map_make(alc);
    f->arg_types = array_make(alc, 4);
    f->class = NULL;
    f->cached_values = NULL;
    f->is_inline = false;
    f->is_static = false;

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

    tok_expect(fc, "(", true, false);
    func->chunk_args = chunk_clone(fc->alc, fc->chunk_parse);
    skip_body(fc);

    if(fc->is_header) {
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        skip_type(fc);
        return;
    }

    char *tkn = tok(fc, true, true, true);
    char *end = "{";

    if (!str_is(tkn, end)) {
        // Has return type
        tok_back(fc);
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        skip_type(fc);
        tok_expect(fc, end, true, true);
    }

    Chunk *chunk_end = chunk_clone(fc->alc, fc->chunk_parse);
    chunk_end->i = chunk_end->scope_end_i;
    func->scope->chunk_end = chunk_end;

    func->chunk_body = chunk_clone(fc->alc, fc->chunk_parse);
    skip_body(fc);
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
