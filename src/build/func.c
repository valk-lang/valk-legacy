
#include "../all.h"

Func* func_make(Allocator* alc, Fc* fc, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = fc->b;
    f->fc = fc;
    f->scope = scope_make(alc, fc->scope);
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;
    f->args = map_make(alc);
    f->class = NULL;
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

    char *tkn = tok(fc, true, true, true);
    char *end = fc->is_header ? ";" : "{";

    if (!str_is(tkn, end)) {
        // Has return type
        tok_back(fc);
        func->chunk_rett = chunk_clone(fc->alc, fc->chunk_parse);
        skip_type(fc);
        tok_expect(fc, end, true, true);
    }

    if (!fc->is_header) {
        func->chunk_body = chunk_clone(fc->alc, fc->chunk_parse);
        skip_body(fc);
    }
}
