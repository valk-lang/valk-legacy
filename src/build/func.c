
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
